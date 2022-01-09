#include "SystemEnvironment.h"
#include "RenderInterface/Shader.h"

using namespace WhiteEngine::System;

GlobalEnvironment* Environment = nullptr;

struct GlobalEnvironmentGurad {
	std::unique_ptr<GlobalEnvironment> pEnvironment;
	GlobalEnvironmentGurad()
		:pEnvironment(std::make_unique<GlobalEnvironment>()){

		Environment = pEnvironment.get();

		//先初始化时间[函数静态实例]
		static platform::chrono::NinthTimer Timer;
		pEnvironment->Timer = &Timer;

		//Task调度器
		static white::threading::TaskScheduler Scheduler;
		pEnvironment->Scheduler = &Scheduler;

		pEnvironment->Gamma = 2.2f;

		//编译Shader
		platform::Render::CompileShaderMap();
	}

	~GlobalEnvironmentGurad() {
		pEnvironment.reset();
	}
};

std::shared_ptr<void> WhiteEngine::System::InitGlobalEnvironment() {
	return std::make_shared<GlobalEnvironmentGurad>();
}