#include "skybox.h"

#include "../utilities.h"
#include "filesystem"

#include <algorithm>
#include <array>

Skybox::Skybox(Device& device, const std::string& folderPath): m_Device(device) {
	m_SkyboxModel = Model::CreateModelFromFile(m_Device, "../../assets/models/cube.obj");

	std::array<std::string, 6> filepaths;
	int fileCount = 0;
	auto dir      = std::filesystem::directory_iterator(folderPath);
	for(int i = 0; i < 6; i++) {
		fileCount++;
		if(std::filesystem::is_regular_file(dir->path())) { filepaths[i] = dir->path().string(); }
		dir++;
	}
	ASSERT(fileCount == 6);
	std::sort(filepaths.begin(), filepaths.end());

	m_Cubemap.CreateImageFromTexture(filepaths);
}