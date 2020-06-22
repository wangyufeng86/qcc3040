/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */
/*
 * \file
 *
 * Interface to additional functions provided in crt0
 *
 */

/**
 * Stop at a branch to self instruction
 * The branch is preceded by a simulation only instruction
 * which will cause simulators to stop, but which has no
 * effect on real hardware
 * \param status the return code (ignored but compatible with ISO C)
 */
extern void exit(int status);
