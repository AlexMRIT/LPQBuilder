#include "LpqEngine.h"

int main()
{
	LpqEngine lpqEngine;

	lpqEngine.compress("gamedata/mesh", "mesh");
	lpqEngine.compress("gamedata/shader", "shader");

	std::vector<bhledict_t*> bhl_data = lpqEngine.load();

	std::vector<lpqedict_t*> m_shaders = lpqEngine.find_lpq_files(bhl_data, "shader.lpq");
	std::vector<lpqedict_t*> m_meshes = lpqEngine.find_lpq_files(bhl_data, "mesh.lpq");

	file_buffer* m_shader = lpqEngine.decompress(m_shaders, "shaders.fx");
	file_buffer* m_model = lpqEngine.decompress(m_meshes, "m_hbc000.obj");
	file_buffer* m_texture = lpqEngine.decompress(m_meshes, "m_hbc0000000.png");

	lpqEngine.save_file(*m_shader);
	lpqEngine.save_file(*m_model);
	lpqEngine.save_file(*m_texture);

	system("pause");

	return 0;
}