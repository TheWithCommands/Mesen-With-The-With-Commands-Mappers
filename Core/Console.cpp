#include "stdafx.h"
#include "Console.h"
#include "MapperFactory.h"
#include "../Utilities/Timer.h"

Console* Console::Instance = nullptr;
IMessageManager* Console::MessageManager = nullptr;
uint32_t Console::Flags = 0;
uint32_t Console::CurrentFPS = 0;
SimpleLock Console::PauseLock;
SimpleLock Console::RunningLock;

Console::Console(wstring filename)
{
	_romFilename = filename;

	_mapper = MapperFactory::InitializeFromFile(filename);
	_memoryManager.reset(new MemoryManager(_mapper));
	_cpu.reset(new CPU(_memoryManager.get()));
	_ppu.reset(new PPU(_memoryManager.get()));	
	_apu.reset(new APU(_memoryManager.get()));

	_controlManager.reset(new ControlManager());

	_memoryManager->RegisterIODevice(_mapper.get());
	_memoryManager->RegisterIODevice(_ppu.get());
	_memoryManager->RegisterIODevice(_apu.get());
	_memoryManager->RegisterIODevice(_controlManager.get());

	ResetComponents(false);

	Console::PauseLock.Release();
	Console::RunningLock.Release();
	Console::Instance = this;
}

Console::~Console()
{
	Movie::Stop();
	Console::PauseLock.Release();
	Console::RunningLock.Release();
	if(Console::Instance == this) {
		Console::Instance = nullptr;
	}
}

void Console::Reset()
{
	Movie::Stop();
	if(Instance) {
		Instance->ResetComponents(true);
	}
}

void Console::ResetComponents(bool softReset)
{
	_cpu->Reset(softReset);
	_ppu->Reset();
	_apu->Reset();
}

void Console::Stop()
{
	_stop = true;
}

void Console::Pause()
{
	Console::PauseLock.Acquire();
	
	//Spin wait until emu pauses
	Console::RunningLock.Acquire();
}

void Console::Resume()
{
	Console::RunningLock.Release();
	Console::PauseLock.Release();
}

void Console::SetFlags(int flags)
{
	Console::Flags |= flags;
}

void Console::ClearFlags(int flags)
{
	Console::Flags &= ~flags;
}

bool Console::CheckFlag(int flag)
{
	return (Console::Flags & flag) == flag;
}

void Console::RegisterMessageManager(IMessageManager* messageManager)
{
	Console::MessageManager = messageManager;
}

void Console::DisplayMessage(wstring message)
{
	if(Console::MessageManager) {
		Console::MessageManager->DisplayMessage(message);
	}
}

uint32_t Console::GetFPS()
{
	return Console::CurrentFPS;
}

void Console::Run()
{
	Timer clockTimer;
	Timer fpsTimer;
	uint32_t lastFrameCount = 0;
	double elapsedTime = 0;
	double targetTime = 16.6666666666666666;
	
	Console::RunningLock.Acquire();

	while(true) { 
		uint32_t executedCycles = _cpu->Exec();
		_ppu->Exec();
		bool frameDone = _apu->Exec(executedCycles);

		if(frameDone) {
			_cpu->EndFrame();
			_ppu->EndFrame();

			if(CheckFlag(EmulationFlags::LimitFPS) && frameDone) {
				elapsedTime = clockTimer.GetElapsedMS();
				while(targetTime > elapsedTime) {
					if(targetTime - elapsedTime > 2) {
						std::this_thread::sleep_for(std::chrono::duration<int, std::milli>((int)(targetTime - elapsedTime - 1)));
					}
					elapsedTime = clockTimer.GetElapsedMS();
				}
			}
			
			if(!Console::PauseLock.IsFree()) {
				//Need to temporarely pause the emu (to save/load a state, etc.)
				Console::RunningLock.Release();

				//Spin wait until we are allowed to start again
				Console::PauseLock.WaitForRelease();

				Console::RunningLock.Acquire();
			}

			clockTimer.Reset();
			
			if(_stop) {
				_stop = false;
				break;
			}
		}

		if(fpsTimer.GetElapsedMS() > 1000) {
			uint32_t frameCount = _ppu->GetFrameCount();
			Console::CurrentFPS = (int)(std::round((double)(frameCount - lastFrameCount) / (fpsTimer.GetElapsedMS() / 1000)));
			lastFrameCount = frameCount;
			fpsTimer.Reset();
		}
	}
	Console::RunningLock.Release();
}

void Console::SaveState(wstring filename)
{
	ofstream file(filename, ios::out | ios::binary);

	if(file) {
		Console::Pause();
		Console::SaveState(file);
		Console::Resume();
		file.close();

		Console::DisplayMessage(L"State saved.");
	}
}

bool Console::LoadState(wstring filename)
{
	ifstream file(filename, ios::out | ios::binary);

	if(file) {
		Console::Pause();
		Console::LoadState(file);
		Console::Resume();
		file.close();

		Console::DisplayMessage(L"State loaded.");
		return true;
	}

	Console::DisplayMessage(L"Slot is empty.");
	return false;
}

void Console::SaveState(ostream &saveStream)
{
	if(Instance) {
		Instance->_cpu->SaveSnapshot(&saveStream);
		Instance->_ppu->SaveSnapshot(&saveStream);
		Instance->_memoryManager->SaveSnapshot(&saveStream);
		Instance->_mapper->SaveSnapshot(&saveStream);
		Instance->_apu->SaveSnapshot(&saveStream);
		Instance->_controlManager->SaveSnapshot(&saveStream);
	}
}

void Console::LoadState(istream &loadStream)
{
	if(Instance) {
		Instance->_cpu->LoadSnapshot(&loadStream);
		Instance->_ppu->LoadSnapshot(&loadStream);
		Instance->_memoryManager->LoadSnapshot(&loadStream);
		Instance->_mapper->LoadSnapshot(&loadStream);
		Instance->_apu->LoadSnapshot(&loadStream);
		Instance->_controlManager->LoadSnapshot(&loadStream);
	}
}

void Console::LoadState(uint8_t *buffer, uint32_t bufferSize)
{
	stringstream stream;
	stream.write((char*)buffer, bufferSize);
	stream.seekg(0, ios::beg);
	LoadState(stream);
}

bool Console::RunTest(uint8_t *expectedResult)
{
	Timer timer;
	uint8_t maxWait = 60;
	uint8_t* lastFrameBuffer = new uint8_t[256 * 240 * 4];
	while(true) {
		uint32_t executedCycles = _cpu->Exec();
		_ppu->Exec();

		if(_apu->Exec(executedCycles)) {
			_cpu->EndFrame();
			_ppu->EndFrame();
		}

		if(timer.GetElapsedMS() > 100) {
			if(memcmp(_ppu->GetFrameBuffer(), expectedResult, 256 * 240 * 4) == 0) {
				return true;
			}

			timer.Reset();

			if(memcmp(lastFrameBuffer, _ppu->GetFrameBuffer(), 256 * 240 * 4) != 0) {
				memcpy(lastFrameBuffer, _ppu->GetFrameBuffer(), 256 * 240 * 4);
				maxWait = 60;
			}

			maxWait--;
			if(maxWait == 0) {
				return false;
			}
		}
	}
	
	delete[] lastFrameBuffer;

	return false;
}

void Console::SaveTestResult()
{
	wstring filename = _romFilename + L".trt";
	
	ofstream testResultFile(filename, ios::out | ios::binary);

	if(!testResultFile) {
		throw std::exception("File could not be opened");
	}

	uint8_t* buffer = new uint8_t[256 * 240 * 4];
	memcpy(buffer, _ppu->GetFrameBuffer(), 256 * 240 * 4);

	testResultFile.write((char *)buffer, 256 * 240 * 4);
	
	testResultFile.close();

	delete[] buffer;
}