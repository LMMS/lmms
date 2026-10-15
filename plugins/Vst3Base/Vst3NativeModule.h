/* Platform module loading shared by the host and isolated scanner.
 * Copyright (c) 2026 LMMS developers. Licensed under GPL-2.0-or-later.
 */
#ifndef LMMS_VST3_NATIVE_MODULE_H
#define LMMS_VST3_NATIVE_MODULE_H

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <dlfcn.h>
#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif
#ifdef Q_OS_LINUX
#include <QDebug>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#endif

namespace lmms::vst3
{
#ifdef Q_OS_LINUX
//! Wine sometimes tears a yabridge host down only halfway: the main thread
//! exits but a socket thread keeps spinning at 100% CPU, so the process
//! never finishes. yabridge waits for its host when a module unloads, which
//! then hangs forever, and a host orphaned this way outlives LMMS. While one
//! of these is alive, SIGKILL the yabridge hosts among our children that stay
//! in that state - a process whose main thread is gone can only be exiting.
class Vst3StuckHostReaper
{
public:
	Vst3StuckHostReaper() : m_thread{[this] { run(); }} {}
	Vst3StuckHostReaper(const Vst3StuckHostReaper&) = delete;
	Vst3StuckHostReaper& operator=(const Vst3StuckHostReaper&) = delete;
	~Vst3StuckHostReaper()
	{
		{
			std::lock_guard<std::mutex> lock{m_mutex};
			m_done = true;
		}
		m_wake.notify_one();
		m_thread.join();
	}

	//! yabridge host processes whose parent is @p parent; with @p stuckOnly
	//! only those whose main thread exited while other threads still run
	static std::vector<pid_t> yabridgeHosts(pid_t parent, bool stuckOnly)
	{
		std::vector<pid_t> result;
		DIR* proc = opendir("/proc");
		if (!proc) { return result; }
		while (const dirent* entry = readdir(proc))
		{
			char* end = nullptr;
			const long pid = std::strtol(entry->d_name, &end, 10);
			if (pid <= 0 || *end != '\0') { continue; }

			char path[64];
			std::snprintf(path, sizeof(path), "/proc/%ld/stat", pid);
			FILE* file = std::fopen(path, "r");
			if (!file) { continue; }
			char stat[1024];
			const size_t size = std::fread(stat, 1, sizeof(stat) - 1, file);
			std::fclose(file);
			stat[size] = '\0';

			// "pid (comm) state ppid ...", where comm may contain anything
			const char* open = std::strchr(stat, '(');
			const char* close = std::strrchr(stat, ')');
			if (!open || !close || close < open) { continue; }
			const std::string comm(open + 1, close);
			// group hosts may serve other plugins: only take them when stuck
			const bool group = comm.rfind("yabridge-group", 0) == 0;
			if (comm.rfind("yabridge-host", 0) != 0 && !(group && stuckOnly)) { continue; }

			char state = 0;
			int ppid = 0;
			long threads = 0;
			// fields after comm: state(3) ppid(4) ... num_threads(20)
			if (std::sscanf(close + 1, " %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %ld",
				&state, &ppid, &threads) != 3) { continue; }
			if (ppid != parent) { continue; }
			if (stuckOnly && !(state == 'Z' && threads > 1)) { continue; }
			result.push_back(static_cast<pid_t>(pid));
		}
		closedir(proc);
		return result;
	}

private:
	void run()
	{
		using namespace std::chrono_literals;
		// a normal exit passes through this state for a moment, too
		constexpr int StuckPolls = 4;
		std::map<pid_t, int> stuck;
		std::unique_lock<std::mutex> lock{m_mutex};
		while (!m_wake.wait_for(lock, 250ms, [this] { return m_done; }))
		{
			std::map<pid_t, int> polls;
			for (const pid_t pid : yabridgeHosts(getpid(), true))
			{
				const auto it = stuck.find(pid);
				const int count = (it == stuck.end() ? 0 : it->second) + 1;
				if (count < StuckPolls) { polls[pid] = count; }
				else
				{
					qWarning() << "VST3: killing stuck yabridge host" << pid;
					::kill(pid, SIGKILL);
				}
			}
			stuck = std::move(polls);
		}
	}

	std::mutex m_mutex;
	std::condition_variable m_wake;
	bool m_done = false;
	std::thread m_thread;
};
#endif

class Vst3NativeModule
{
public:
	Vst3NativeModule() = default;
	Vst3NativeModule(const Vst3NativeModule&) = delete;
	Vst3NativeModule& operator=(const Vst3NativeModule&) = delete;
	~Vst3NativeModule()
	{
#ifdef Q_OS_LINUX
		// yabridge stops and waits for its Wine host in ModuleExit()
		const Vst3StuckHostReaper reaper;
#endif
		if (m_entered)
		{
			if (auto exit = reinterpret_cast<bool (*)()>(symbol(exitName()))) { exit(); }
		}
		if (m_handle) { dlclose(m_handle); }
#ifdef Q_OS_MACOS
		if (m_bundle) { CFRelease(m_bundle); }
#endif
	}

#ifdef Q_OS_MACOS
	static CFBundleRef createBundle(const QString& path)
	{
		const auto bytes = QFile::encodeName(QFileInfo{path}.absoluteFilePath());
		auto url = CFURLCreateFromFileSystemRepresentation(nullptr,
			reinterpret_cast<const UInt8*>(bytes.constData()), bytes.size(), true);
		if (!url) { return nullptr; }
		auto bundle = CFBundleCreate(nullptr, url);
		CFRelease(url);
		return bundle;
	}

	static QString executablePath(const QString& path)
	{
		auto bundle = createBundle(path);
		if (!bundle) { return {}; }
		auto url = CFBundleCopyExecutableURL(bundle);
		CFRelease(bundle);
		if (!url) { return {}; }
		auto absolute = CFURLCopyAbsoluteURL(url);
		CFRelease(url);
		if (!absolute) { return {}; }
		auto string = CFURLCopyFileSystemPath(absolute, kCFURLPOSIXPathStyle);
		CFRelease(absolute);
		if (!string) { return {}; }
		const auto length = CFStringGetLength(string);
		QString result(length, Qt::Uninitialized);
		CFStringGetCharacters(string, CFRangeMake(0, length), reinterpret_cast<UniChar*>(result.data()));
		CFRelease(string);
		return result;
	}
#endif

	bool open(const QString& path, QString* error = nullptr)
	{
		QString binary = path;
		void* entryArgument = nullptr;
#ifdef Q_OS_MACOS
		QString bundlePath = QFileInfo{path}.absoluteFilePath();
		const int contents = bundlePath.indexOf(".vst3/Contents/");
		if (contents >= 0) { bundlePath.truncate(contents + 5); }
		m_bundle = createBundle(bundlePath);
		if (!m_bundle)
		{
			if (error) { *error = QString("Invalid VST3 bundle: %1").arg(path); }
			return false;
		}
		binary = executablePath(bundlePath);
		// dyld can load a damaged signed image successfully, then macOS kills
		// the entire host when a bad code page is first touched. Validate the
		// native executable before running any plugin code (including entry).
		auto url = CFBundleCopyBundleURL(m_bundle);
		SecStaticCodeRef code = nullptr;
		auto status = SecStaticCodeCreateWithPath(url, kSecCSDefaultFlags, &code);
		CFRelease(url);
		if (status == errSecSuccess)
		{
			status = SecStaticCodeCheckValidity(code, kSecCSDoNotValidateResources, nullptr);
		}
		if (code) { CFRelease(code); }
		// Preserve support for unsigned development plugins where macOS allows
		// them. This checks signature integrity, not notarization or publisher.
		if (status != errSecSuccess && status != errSecCSUnsigned)
		{
			if (error)
			{
				*error = QString("VST3 code signature validation failed for \"%1\" (macOS error %2). "
					"Reinstall the plugin from its original installer before loading it.")
					.arg(bundlePath).arg(status);
			}
			return false;
		}
		entryArgument = m_bundle;
#endif
		// Keep code mapped while asynchronous plugin threads unwind after exit.
		m_handle = dlopen(QFile::encodeName(binary).constData(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
		if (!m_handle)
		{
			if (error) { *error = QString("dlopen failed: %1").arg(dlerror()); }
			return false;
		}
#ifndef Q_OS_MACOS
		entryArgument = m_handle;
#endif
		auto entry = reinterpret_cast<bool (*)(void*)>(symbol(entryName()));
#ifdef Q_OS_MACOS
		if (!entry || !symbol(exitName()))
		{
			if (error) { *error = QString("Missing VST3 bundle entry points: %1").arg(path); }
			return false;
		}
#endif
		if (entry)
		{
			if (!entry(entryArgument))
			{
				if (error) { *error = QString("VST3 module entry failed: %1").arg(path); }
				return false;
			}
			m_entered = true;
		}
		return true;
	}

	void* symbol(const char* name) const { return dlsym(m_handle, name); }

private:
	static const char* entryName()
	{
#ifdef Q_OS_MACOS
		return "bundleEntry";
#else
		return "ModuleEntry";
#endif
	}
	static const char* exitName()
	{
#ifdef Q_OS_MACOS
		return "bundleExit";
#else
		return "ModuleExit";
#endif
	}
	void* m_handle = nullptr;
	bool m_entered = false;
#ifdef Q_OS_MACOS
	CFBundleRef m_bundle = nullptr;
#endif
};
} // namespace lmms::vst3
#endif
