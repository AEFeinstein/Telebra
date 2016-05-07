/*
 * webpages.h
 *
 *  Created on: May 7, 2016
 *      Author: adam
 */

#ifndef WEBPAGES_H_
#define WEBPAGES_H_

void bad_request(int32_t);
void cannot_execute(int32_t);
void headers(int32_t, const char*);
void not_found(int32_t);
void unimplemented(int32_t);

#endif /* WEBPAGES_H_ */
