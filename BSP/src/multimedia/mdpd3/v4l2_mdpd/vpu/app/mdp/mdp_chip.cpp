
extern "C"
{
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "mdp_chip.h"


volatile enum mdp_chip g_mdp_chip = MDP_UNKNOWN_CHIP;

extern "C"
int mdp_support(void)
{
	size_t ret;
	FILE *stream;
	char buf[128] = { 0 };
	int i, cnt;
	const char *fn[] = {
		"/proc/device-tree/vcu@0/compatible",
		"/proc/device-tree/vcu@1/compatible",
		"/proc/device-tree/vpu@0/compatible", /* compatible 2701 */
		"/proc/device-tree/soc/vcu@0/compatible", /* compatilbe with k4.4, vcu is child node of soc. */
		"/proc/device-tree/model"
	};

	cnt = sizeof(fn)/sizeof(fn[0]);
	for (i = 0; i < cnt; i++) {
		stream = fopen(fn[i], "r");
		if (NULL == stream) {
			continue;
		}

		ret = fread(buf, sizeof(buf), sizeof(char), stream);
		fclose(stream);

		//fprintf(stderr, "[mdp] fread %s, str=%s\n", fn[i], buf);

		if (strstr(buf, "2712"))
			g_mdp_chip = MDP_2712;
		else if (strstr(buf, "2701"))
			g_mdp_chip = MDP_2701;
		else if (strstr(buf, "6799"))
			g_mdp_chip = MDP_6799;
		else if (strstr(buf, "8173"))
			g_mdp_chip = MDP_8173;
		else if (strstr(buf, "8167"))
			g_mdp_chip = MDP_8167;
		else if (strstr(buf, "8183"))
			g_mdp_chip = MDP_8183;
		else if (strstr(buf, "8512"))
			g_mdp_chip = MDP_8512;

		if (g_mdp_chip != MDP_UNKNOWN_CHIP)
			break;
	}

	if (g_mdp_chip == MDP_UNKNOWN_CHIP) {
		fprintf(stderr, "[mdp] unsupported platform\n");
		for (i = 0; i < cnt; i++) {
			fprintf(stderr, "[mdp] %s\n", fn[i]);
		}
	}

	return (g_mdp_chip == MDP_UNKNOWN_CHIP) ? 0 : 1;
}

