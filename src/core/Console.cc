#include "Console.hh"
#include "Logging.hh"

#include <linenoise/linenoise.h>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <mutex>

namespace sp
{
	ConsoleManager GConsoleManager;

	void LinenoiseCompletionCallback(const char *buf, linenoiseCompletions *lc);

	namespace logging
	{
		void GlobalLogOutput(Level lvl, const string &line)
		{
			static std::mutex mut;
			mut.lock();
			GConsoleManager.AddLog(lvl, line);
			mut.unlock();
		}
	}

	CVarBase::CVarBase(const string &name, const string &description)
		: name(name), description(description)
	{
		GConsoleManager.AddCVar(this);
	}

	ConsoleManager::ConsoleManager()
	{
		inputThread = std::thread([&] { this->InputLoop(); });
		inputThread.detach();
	}

	void ConsoleManager::AddCVar(CVarBase *cvar)
	{
		cvars[boost::algorithm::to_lower_copy(cvar->GetName())] = cvar;
	}

	void ConsoleManager::InputLoop()
	{
		char *str;
		std::unique_lock<std::mutex> ulock(inputLock, std::defer_lock);

		linenoiseHistorySetMaxLen(256);
		linenoiseSetCompletionCallback(LinenoiseCompletionCallback);

		while ((str = linenoise("sp> ")) != nullptr)
		{
			if (*str == '\0')
			{
				linenoiseFree(str);
				continue;
			}

			ConsoleInputLine line;
			line.text = string(str);

			ulock.lock();
			inputLines.push(&line);
			line.handled.wait(ulock);
			ulock.unlock();

			linenoiseHistoryAdd(str);
			linenoiseFree(str);
		}
	}

	void ConsoleManager::AddLog(logging::Level lvl, const string &line)
	{
		outputLines.push_back(
		{
			lvl,
			line
		});
	}

	void ConsoleManager::Update()
	{
		std::unique_lock<std::mutex> ulock(inputLock, std::defer_lock);

		while (!inputLines.empty())
		{
			if (!ulock.try_lock())
				return;

			auto line = inputLines.front();
			inputLines.pop();
			ParseAndExecute(line->text);
			ulock.unlock();

			line->handled.notify_all();
		}
	}

	void ConsoleManager::ParseAndExecute(const string &line)
	{
		if (line == "")
			return;

		if (history.size() == 0 || history[history.size() - 1] != line)
			history.push_back(line);

		std::stringstream stream(line);
		string varName, value;
		stream >> varName;
		getline(stream, value);

		if (varName == "list")
		{
			for (auto &kv : cvars)
			{
				auto cvar = kv.second;
				logging::ConsoleWrite(logging::Level::Log, " > %s = %s", cvar->GetName(), cvar->StringValue());
				logging::ConsoleWrite(logging::Level::Log, " >   %s", cvar->GetDescription());
			}
			return;
		}

		auto cvarit = cvars.find(boost::algorithm::to_lower_copy(varName));
		if (cvarit != cvars.end())
		{
			auto cvar = cvarit->second;

			if (value.length() > 0)
			{
				cvar->SetFromString(value);
			}

			logging::ConsoleWrite(logging::Level::Log, " > %s = %s", cvar->GetName(), cvar->StringValue());

			if (value.length() == 0)
			{
				logging::ConsoleWrite(logging::Level::Log, " >   %s", cvar->GetDescription());
			}
		}
		else
		{
			logging::ConsoleWrite(logging::Level::Log, " > '%s' undefined", varName);
		}
	}

	string ConsoleManager::AutoComplete(const string &input)
	{
		auto it = cvars.upper_bound(boost::algorithm::to_lower_copy(input));
		if (it == cvars.end())
			return input;

		auto cvar = it->second;
		return cvar->GetName() + " ";
	}

	vector<string> ConsoleManager::AllCompletions(const string &rawInput)
	{
		vector<string> results;

		auto input = boost::algorithm::to_lower_copy(rawInput);
		auto it = cvars.lower_bound(input);

		for (; it != cvars.end(); it++)
		{
			auto cvar = it->second;
			auto name = cvar->GetName();

			if (boost::starts_with(boost::algorithm::to_lower_copy(name), input))
				results.push_back(name);
			else
				break;
		}

		return results;
	}

	void LinenoiseCompletionCallback(const char *buf, linenoiseCompletions *lc)
	{
		auto completions = GConsoleManager.AllCompletions(string(buf));
		for (auto str : completions)
		{
			linenoiseAddCompletion(lc, str.c_str());
		}
	}
}
