#include "SystemEnvironment.h"
#include "RenderInterface/Shader.h"

using namespace WhiteEngine::System;

GlobalEnvironment* Environment = nullptr;

struct GlobalEnvironmentGurad {
	std::unique_ptr<GlobalEnvironment> pEnvironment;
	GlobalEnvironmentGurad()
		:pEnvironment(std::make_unique<GlobalEnvironment>()){

		Environment = pEnvironment.get();

		//�ȳ�ʼ��ʱ��[������̬ʵ��]
		static platform::chrono::NinthTimer Timer;
		pEnvironment->Timer = &Timer;

		//Task������
		static white::threading::TaskScheduler Scheduler;
		pEnvironment->Scheduler = &Scheduler;

		pEnvironment->Gamma = 2.2f;

		//����Shader
		platform::Render::CompileShaderMap();
	}

	~GlobalEnvironmentGurad() {
		pEnvironment.reset();
	}
};

std::shared_ptr<void> WhiteEngine::System::InitGlobalEnvironment() {
	return std::make_shared<GlobalEnvironmentGurad>();
}