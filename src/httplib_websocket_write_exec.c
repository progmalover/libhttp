/* 
 * Copyright (c) 2016 Lammert Bies
 * Copyright (c) 2013-2016 the Civetweb developers
 * Copyright (c) 2004-2013 Sergey Lyubka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#include "libhttp-private.h"



/*
 * int XX_httplib_websocket_write_exec( struct mg_connection *conn, int opcode, const char *data, size_t dataLen, uint32_t masking_key );
 *
 * The function XX_httplib_websocket_write_exec() does the heavy lifting in
 * writing data over a websocket connectin to a remote peer.
 */

#if defined(USE_WEBSOCKET)

int XX_httplib_websocket_write_exec( struct mg_connection *conn, int opcode, const char *data, size_t dataLen, uint32_t masking_key ) {

	unsigned char header[14];
	size_t headerLen = 1;

	int retval = -1;

	header[0] = 0x80 + (opcode & 0xF);

	/* Frame format: http://tools.ietf.org/html/rfc6455#section-5.2 */
	if (dataLen < 126) {
		/* inline 7-bit length field */
		header[1] = (unsigned char)dataLen;
		headerLen = 2;
	} else if (dataLen <= 0xFFFF) {
		/* 16-bit length field */
		uint16_t len = htons((uint16_t)dataLen);
		header[1] = 126;
		memcpy(header + 2, &len, 2);
		headerLen = 4;
	} else {
		/* 64-bit length field */
		uint32_t len1 = htonl((uint64_t)dataLen >> 32);
		uint32_t len2 = htonl(dataLen & 0xFFFFFFFF);
		header[1] = 127;
		memcpy(header + 2, &len1, 4);
		memcpy(header + 6, &len2, 4);
		headerLen = 10;
	}

	if (masking_key) {
		/* add mask */
		header[1] |= 0x80;
		memcpy(header + headerLen, &masking_key, 4);
		headerLen += 4;
	}


	/* Note that POSIX/Winsock's send() is threadsafe
	 * http://stackoverflow.com/questions/1981372/are-parallel-calls-to-send-recv-on-the-same-socket-valid
	 * but mongoose's mg_printf/mg_write is not (because of the loop in
	 * push(), although that is only a problem if the packet is large or
	 * outgoing buffer is full). */
	(void)mg_lock_connection(conn);
	retval = mg_write(conn, header, headerLen);
	if (dataLen > 0) retval = mg_write(conn, data, dataLen);
	mg_unlock_connection(conn);

	return retval;

}  /* XX_httplib_websocket_write_exec */

#endif /* !USE_WEBSOCKET */