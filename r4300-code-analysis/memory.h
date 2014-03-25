/*
 * memory.h
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#ifndef MEMORY_H_
#define MEMORY_H_


#define sl(mot) \
		( \
				((mot & 0x000000FF) << 24) | \
				((mot & 0x0000FF00) <<  8) | \
				((mot & 0x00FF0000) >>  8) | \
				((mot & 0xFF000000) >> 24) \
		)

#endif /* MEMORY_H_ */
