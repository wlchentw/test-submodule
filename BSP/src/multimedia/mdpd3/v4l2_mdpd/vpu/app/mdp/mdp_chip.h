
#ifndef MDP_CHIP_H__
#define MDP_CHIP_H__

enum mdp_chip {
	MDP_2701 = 0x2701,
	MDP_8173 = 0x8173,
	MDP_2712 = 0x2712,
	MDP_6799 = 0x6799,
	MDP_8167 = 0x8167,
	MDP_8183 = 0x8183,
	MDP_8512 = 0x8512,
	MDP_UNKNOWN_CHIP = 0
};

#define MDP_CHIP()    (g_mdp_chip)
#define IS_MDP_2701() (g_mdp_chip == MDP_2701)
#define IS_MDP_8173() (g_mdp_chip == MDP_8173)
#define IS_MDP_2712() (g_mdp_chip == MDP_2712)
#define IS_MDP_6799() (g_mdp_chip == MDP_6799)
#define IS_MDP_8167() (g_mdp_chip == MDP_8167)
#define IS_MDP_8183() (g_mdp_chip == MDP_8183)
#define IS_MDP_8512() (g_mdp_chip == MDP_8512)

extern volatile enum mdp_chip g_mdp_chip;

#endif
