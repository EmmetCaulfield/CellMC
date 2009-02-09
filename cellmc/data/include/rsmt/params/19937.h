#ifndef RSMT_PARAMS19937_H
#define RSMT_PARAMS19937_H

#define RM_POS1	122
#define RM_SL1	18
#define RM_SL2	1
#define RM_SR1	11
#define RM_SR2	1
#define RM_MSK1	0xdfffffefU
#define RM_MSK2	0xddfecb7fU
#define RM_MSK3	0xbffaffffU
#define RM_MSK4	0xbffffff6U
#define RM_PARITY1	0x00000001U
#define RM_PARITY2	0x00000000U
#define RM_PARITY3	0x00000000U
#define RM_PARITY4	0x13c9e684U


/* PARAMETERS FOR ALTIVEC */
#if defined(__APPLE__)	/* For OSX */
    #define RM_ALTI_SL1	(vector unsigned int)(SL1, SL1, SL1, SL1)
    #define RM_ALTI_SR1	(vector unsigned int)(SR1, SR1, SR1, SR1)
    #define RM_ALTI_MSK	(vector unsigned int)(MSK1, MSK2, MSK3, MSK4)
    #define RM_ALTI_MSK64 \
	(vector unsigned int)(MSK2, MSK1, MSK4, MSK3)
    #define RM_ALTI_SL2_PERM \
	(vector unsigned char)(1,2,3,23,5,6,7,0,9,10,11,4,13,14,15,8)
    #define RM_ALTI_SL2_PERM64 \
	(vector unsigned char)(1,2,3,4,5,6,7,31,9,10,11,12,13,14,15,0)
    #define RM_ALTI_SR2_PERM \
	(vector unsigned char)(7,0,1,2,11,4,5,6,15,8,9,10,17,12,13,14)
    #define RM_ALTI_SR2_PERM64 \
	(vector unsigned char)(15,0,1,2,3,4,5,6,17,8,9,10,11,12,13,14)
#else	/* For OTHER OSs(Linux?) */
    #define RM_ALTI_SL1	{SL1, SL1, SL1, SL1}
    #define RM_ALTI_SR1	{SR1, SR1, SR1, SR1}
    #define RM_ALTI_MSK	{MSK1, MSK2, MSK3, MSK4}
    #define RM_ALTI_MSK64	{MSK2, MSK1, MSK4, MSK3}
    #define RM_ALTI_SL2_PERM	{1,2,3,23,5,6,7,0,9,10,11,4,13,14,15,8}
    #define RM_ALTI_SL2_PERM64	{1,2,3,4,5,6,7,31,9,10,11,12,13,14,15,0}
    #define RM_ALTI_SR2_PERM	{7,0,1,2,11,4,5,6,15,8,9,10,17,12,13,14}
    #define RM_ALTI_SR2_PERM64	{15,0,1,2,3,4,5,6,17,8,9,10,11,12,13,14}
#endif	/* For OSX */
#define RM_IDSTR	"SFMT-19937:122-18-1-11-1:dfffffef-ddfecb7f-bffaffff-bffffff6"

#endif /* RSMT_PARAMS_19937_H */
