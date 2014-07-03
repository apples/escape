/*******************************************************************************
 * Inugami - An OpenGL framework designed for rapid game development
 * Version: 0.3.0
 * https://github.com/DBRalir/Inugami
 *
 * Copyright (c) 2012 Jeramy Harrison <dbralir@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "game.hpp"
#include "meta.hpp"

#include "inugami/exception.hpp"

#include <fstream>
#include <iostream>
#include <exception>
#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Inugami;

void dumpProfiles();
void errorMessage(const char*);

int main(int argc, char* argv[])
{
    profiler = new Profiler();
    auto prof = profiler->scope("Main");

    std::ofstream logfile("log.txt");
    logger = new Logger<>(logfile);

    Game::RenderParams renparams;
    renparams.fsaaSamples = 4;

    {
        std::unordered_map<std::string, std::function<void()>> argf = {
              {"--fullscreen", [&]{renparams.fullscreen=true;}}
            , {"--windowed",   [&]{renparams.fullscreen=false;}}
            , {"--vsync",      [&]{renparams.vsync=true;}}
            , {"--no-vsync",   [&]{renparams.vsync=false;}}
        };

        while (*++argv)
        {
            auto iter = argf.find(*argv);
            if (iter != end(argf)) iter->second();
        }
    }

    try
    {
        logger->log("Creating Core...");
        Game base(renparams);
        logger->log("Go!");
        base.go();
    }
    catch (const std::exception& e)
    {
        errorMessage(e.what());
        return -1;
    }
    catch (...)
    {
        errorMessage("Unknown error!");
        return -1;
    }

    dumpProfiles();

    return 0;
}

void dumpProfiles()
{
    using Prof = Inugami::Profiler::Profile;

    std::ofstream pfile("profile.txt");

    auto all = profiler->getAll();

    std::function<void(const Prof::Ptr&, std::string)> dumProf;
    dumProf = [&](const Prof::Ptr& in, std::string indent)
    {
        pfile << indent << "Min: " << in->min     << "\n";
        pfile << indent << "Max: " << in->max     << "\n";
        pfile << indent << "Avg: " << in->average << "\n";
        pfile << indent << "Num: " << in->samples << "\n\n";
        for (auto& p : in->getChildren())
        {
            pfile << indent << p.first << ":\n";
            dumProf(p.second, indent+"\t");
        }
    };

    for (auto& p : all)
    {
        pfile << p.first << ":\n";
        dumProf(p.second, "\t");
    }
}

#ifdef _WIN32
LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif

void errorMessage(const char* str)
{
	logger->log(str);

#ifdef _WIN32
	WNDCLASSEX wClass = {};
	wClass.cbClsExtra = 0;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = 0;
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wClass.hIcon = nullptr;
	wClass.hIconSm = nullptr;
	wClass.hInstance = GetModuleHandle(nullptr);
	wClass.lpfnWndProc = WinProc;
	wClass.lpszClassName = "Window Class";
	wClass.lpszMenuName = nullptr;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wClass);

	HWND hWnd = CreateWindowEx(0,
		"Window Class",
		"Error",
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
		200,
		200,
		640,
		480,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr);

	RECT clientArea;
	GetClientRect(hWnd, &clientArea);

	// Create an edit box
	HWND hEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
		ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
		0,
		0,
		clientArea.right,
		clientArea.bottom,
		hWnd,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr);

	SendMessage(hEdit,
		WM_SETFONT,
		(WPARAM)GetStockObject(ANSI_FIXED_FONT),
		MAKELPARAM(false, 0));

	SendMessage(hEdit,
		WM_SETTEXT,
		0,
		(LPARAM)str);

	ShowWindow(hWnd, SW_SHOWDEFAULT);

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}
