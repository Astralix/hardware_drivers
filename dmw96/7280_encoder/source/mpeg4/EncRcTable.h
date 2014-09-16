/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncRcTable.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_RC_TABLE_H__
#define __ENC_RC_TABLE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
static const i32 srcInter[32][31] = {
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{180395, 307195, 343811, 378492, 394518, 412161, 420992, 431098,
		436281, 442224, 445289, 448902, 450769, 452995, 454177, 455615,
		456394, 457331, 457854, 458482, 458832, 459268, 459514, 459814,
		459990, 460216, 460345, 460506, 460603, 460724, 460799},
	{272989, 408367, 432096, 446783, 451457, 455266, 456744, 458116,
		458718, 459317, 459601, 459897, 460048, 460213, 460302, 460404,
		460462, 460527, 460563, 460609, 460636, 460669, 460689, 460712,
		460727, 460746, 460758, 460772, 460781, 460792, 460799},
	{256004, 394807, 423990, 442846, 448964, 453997, 455934, 457708,
		458464, 459206, 459547, 459898, 460070, 460250, 460343, 460446,
		460503, 460568, 460602, 460641, 460665, 460693, 460710, 460730,
		460743, 460757, 460766, 460777, 460785, 460793, 460799},
	{263192, 397687, 427599, 445719, 451068, 455237, 456784, 458188,
		458790, 459385, 459664, 459956, 460099, 460258, 460345, 460441,
		460495, 460556, 460590, 460631, 460655, 460685, 460703, 460724,
		460737, 460752, 460763, 460775, 460783, 460792, 460799},
	{255130, 385003, 418125, 441156, 448326, 453887, 455929, 457732,
		458487, 459212, 459548, 459889, 460061, 460241, 460335, 460436,
		460493, 460557, 460591, 460632, 460657, 460686, 460702, 460725,
		460738, 460753, 460764, 460776, 460783, 460793, 460799},
	{252442, 379805, 413263, 438139, 446432, 452912, 455274, 457342,
		458197, 459034, 459410, 459795, 459980, 460184, 460286, 460399,
		460462, 460530, 460569, 460615, 460641, 460674, 460693, 460718,
		460731, 460749, 460760, 460773, 460781, 460792, 460799},
	{244319, 369431, 403916, 431392, 441647, 450446, 453734, 456577,
		457714, 458793, 459260, 459729, 459953, 460183, 460296, 460416,
		460479, 460551, 460588, 460631, 460654, 460684, 460703, 460723,
		460737, 460754, 460765, 460777, 460785, 460793, 460799},
	{253038, 376301, 408974, 434395, 443628, 451394, 454232, 456703,
		457724, 458716, 459152, 459609, 459829, 460066, 460181, 460321,
		460392, 460475, 460522, 460577, 460609, 460647, 460669, 460698,
		460716, 460738, 460752, 460767, 460778, 460791, 460799},
	{249495, 371895, 404785, 430992, 440862, 449668, 453070, 456069,
		457292, 458461, 458975, 459499, 459747, 460011, 460146, 460293,
		460374, 460465, 460515, 460575, 460608, 460649, 460673, 460701,
		460718, 460739, 460753, 460769, 460780, 460792, 460799},
	{257531, 377635, 408871, 433468, 442572, 450552, 453606, 456261,
		457341, 458400, 458869, 459368, 459604, 459874, 460015, 460179,
		460266, 460371, 460428, 460499, 460543, 460594, 460624, 460663,
		460684, 460713, 460732, 460756, 460769, 460786, 460799},
	{248840, 369165, 401588, 427764, 437923, 447399, 451348, 455025,
		456529, 457966, 458598, 459245, 459552, 459872, 460032, 460213,
		460308, 460411, 460470, 460538, 460576, 460626, 460651, 460685,
		460705, 460729, 460746, 460764, 460776, 460790, 460799},
	{256581, 374550, 405289, 430007, 439524, 448314, 451934, 455306,
		456679, 457984, 458556, 459152, 459442, 459755, 459919, 460100,
		460203, 460324, 460393, 460474, 460520, 460577, 460609, 460652,
		460676, 460706, 460725, 460749, 460767, 460787, 460799},
	{255322, 372448, 403102, 427813, 437525, 446733, 450680, 454467,
		456075, 457611, 458286, 458983, 459315, 459683, 459863, 460066,
		460178, 460304, 460373, 460459, 460508, 460565, 460602, 460646,
		460672, 460706, 460725, 460750, 460765, 460784, 460799},
	{245775, 363994, 396033, 422503, 433114, 443446, 448097, 452817,
		454899, 457013, 457919, 458825, 459253, 459690, 459907, 460137,
		460257, 460382, 460452, 460531, 460574, 460622, 460652, 460687,
		460706, 460731, 460746, 460766, 460776, 460790, 460799},
	{251827, 368131, 398970, 424296, 434377, 444208, 448553, 452946,
		454900, 456875, 457755, 458644, 459063, 459506, 459731, 459974,
		460104, 460253, 460331, 460430, 460480, 460545, 460583, 460631,
		460660, 460696, 460719, 460748, 460763, 460784, 460799},
	{253165, 369765, 400376, 425488, 435474, 445063, 449271, 453502,
		455379, 457260, 458060, 458873, 459251, 459652, 459845, 460067,
		460179, 460316, 460386, 460468, 460518, 460573, 460607, 460652,
		460679, 460709, 460728, 460750, 460767, 460787, 460799},
	{250476, 366928, 397681, 422974, 433144, 443103, 447597, 452183,
		454308, 456505, 457482, 458484, 458942, 459447, 459688, 459952,
		460093, 460246, 460328, 460429, 460483, 460551, 460589, 460637,
		460664, 460699, 460721, 460747, 460765, 460785, 460799},
	{250198, 366980, 397945, 423394, 433554, 443575, 448069, 452658,
		454752, 456917, 457866, 458801, 459224, 459668, 459882, 460098,
		460211, 460348, 460425, 460509, 460553, 460608, 460639, 460675,
		460700, 460726, 460742, 460761, 460774, 460790, 460799},
	{249176, 364574, 395070, 420497, 430765, 441007, 445691, 450629,
		452982, 455520, 456718, 457992, 458582, 459186, 459485, 459813,
		459975, 460163, 460265, 460378, 460441, 460517, 460558, 460614,
		460647, 460687, 460713, 460746, 460763, 460784, 460799},
	{251627, 366697, 397105, 422056, 432078, 442003, 446515, 451212,
		453474, 455862, 456993, 458175, 458719, 459291, 459568, 459865,
		460022, 460201, 460293, 460401, 460462, 460534, 460576, 460631,
		460660, 460695, 460718, 460748, 460764, 460786, 460799},
	{248376, 364811, 395655, 421143, 431416, 441653, 446271, 451149,
		453449, 455907, 457059, 458244, 458781, 459352, 459621, 459918,
		460068, 460238, 460328, 460432, 460489, 460559, 460597, 460646,
		460676, 460711, 460730, 460753, 460768, 460787, 460799},
	{252480, 367074, 396953, 421406, 431285, 441150, 445612, 450380,
		452715, 455254, 456451, 457773, 458397, 459059, 459382, 459736,
		459913, 460116, 460220, 460345, 460412, 460496, 460539, 460602,
		460635, 460681, 460705, 460735, 460757, 460781, 460799}
};

#endif

