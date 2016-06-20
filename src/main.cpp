#include "node_lua.h"

int main(int argc, char* argv[], char* env[])
{
	if (argc <= 1) {
		printf("Error: node-lua needs a lua file to be the entry!\n");
		return -1;
	}
	uint64_t start_time = uv_hrtime();
	node_lua_t node(argc - 1, argv + 1, env);
	double duration = (uv_hrtime() - start_time) / 1e9;
	printf("Process used %.3f sec total!\n", duration);
 	getchar();
	return 0;
}
