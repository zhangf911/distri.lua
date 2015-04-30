/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _WPACKET_H
#define _WPACKET_H

#include "kendynet.h"
#include "packet.h"
#include "kn_common_define.h"
#include "kn_alloc.h"


extern allocator_t g_wpk_allocator;    

typedef struct wpacket
{
	struct packet base;
	uint32_t *len;      //�����ֶ�(ȥ���������ֶ�����ʵ�����ݵĳ���)��buf�еĵ�ַ
	buffer_t writebuf;  //wpos���ڵ�buf
	uint32_t wpos;
}*wpacket_t;

struct rpacket;
typedef struct
{
	buffer_t buf;
	uint32_t wpos;
}write_pos;

wpacket_t wpk_create(uint32_t size);

wpacket_t wpk_create_by_bin(int8_t *addr,uint32_t size);

wpacket_t wpk_copy_create(packet_t);

static inline write_pos wpk_get_writepos(wpacket_t w)
{
    write_pos wp = {w->writebuf,w->wpos};
	return wp;
}

static inline void wpk_rewrite(write_pos *wp,int8_t *addr,uint32_t size)
{
	int8_t *ptr = addr;
	uint32_t copy_size;
	uint32_t pos = wp->wpos;
	while(size){
		copy_size = wp->buf->capacity - pos;
		copy_size = copy_size > size ? size:copy_size;
		memcpy(wp->buf->buf + pos,ptr,copy_size);
		ptr += copy_size;
		size -= copy_size;
		pos += copy_size;
		if(size && pos >= wp->buf->capacity)
		{
			assert(wp->buf->next);
			wp->buf = wp->buf->next;
			pos = 0;
		}
	}
}

static inline void wpk_rewrite_uint8(write_pos *wp,uint8_t value)
{
	wpk_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpk_rewrite_uint16(write_pos *wp,uint16_t value)
{
	value = kn_hton16(value);
	wpk_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpk_rewrite_uint32(write_pos *wp,uint32_t value)
{

	value = kn_hton32(value);
	wpk_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpk_rewrite_uint64(write_pos *wp,uint64_t value)
{
	value = kn_hton64(value);
	wpk_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpk_rewrite_double(write_pos *wp,double value)
{
	wpk_rewrite(wp,(int8_t*)&value,sizeof(value));
}


static inline void wpk_expand(wpacket_t w,uint32_t size)
{
    size = size_of_pow2(size);
    if(size < 64) size = 64;
    w->writebuf->next = buffer_create(size);
    w->writebuf = w->writebuf->next;//buffer_acquire(w->writebuf,w->writebuf->next);
    w->wpos = 0;
}

//��w���������ݿ�����buf��
static inline void wpk_copy(wpacket_t w,buffer_t buf)
{
	int8_t *ptr = buf->buf;
	buffer_t tmp_buf = packet_buf(w);
	uint32_t copy_size;
	uint32_t size = packet_datasize(w);
	uint32_t pos = packet_begpos(w);
	while(size)
	{
		copy_size = tmp_buf->size - pos;
		if(copy_size > size) copy_size = size;
		memcpy(ptr,tmp_buf->buf+pos,copy_size);
		ptr += copy_size;
		size -= copy_size;
		pos = 0;
		tmp_buf = tmp_buf->next;
	}
}

static inline void do_write_copy(wpacket_t w)
{
    /*wpacket����rpacket����ģ�����ִ��дʱ������
    * ִ�����wpacket�͹���ʱ�����rpacket���ٹ���buffer
    */
    uint32_t size = size_of_pow2(packet_datasize(w));
    if(size < 64) size = 64;
    buffer_t tmp = buffer_create(size);
    wpk_copy(w,tmp);
    packet_begpos(w) = 0;
    w->wpos = packet_datasize(w);
    tmp->size = packet_datasize(w);
    if(packet_buf(w)) buffer_release(packet_buf(w));
    //if(w->writebuf) buffer_release(w->writebuf);
    packet_buf(w) = tmp;
    w->len = (uint32_t*)tmp->buf;
    w->writebuf = tmp;//buffer_acquire(NULL,packet_buf(w));
}

static inline void wpk_write(wpacket_t w,int8_t *addr,uint32_t size)
{
	int8_t *ptr = addr;
	uint32_t copy_size;
	if(!w->writebuf){
		do_write_copy(w);
	}
	while(size)
	{
		copy_size = w->writebuf->capacity - w->wpos;
		if(copy_size == 0)
		{
            			wpk_expand(w,size);//�ռ䲻��,��չ
			copy_size = w->writebuf->capacity - w->wpos;
		}
		copy_size = copy_size > size ? size:copy_size;
		memcpy(w->writebuf->buf + w->wpos,ptr,copy_size);
		w->writebuf->size += copy_size;
		if(w->len){
			uint32_t l = kn_ntoh32(*w->len) + copy_size;
			*w->len = kn_hton32(l);
		}		
		w->wpos += copy_size;
		ptr += copy_size;
		size -= copy_size;
		packet_datasize(w) += copy_size;
	}
}

#define CHECK_WRITE(TYPE,VALUE)\
		if(!w->writebuf)\
			do_write_copy(w);\
		if(likely(w->writebuf->capacity - w->wpos >= sizeof(TYPE))){\
			uint32_t pos = w->wpos;\
			uint32_t size = sizeof(TYPE);\
			w->wpos += size;\
			*((TYPE*)(&w->writebuf->buf[pos])) = VALUE;\
			packet_datasize(w) += size;\
			if(w->len){\
				uint32_t l = kn_ntoh32(*w->len) + size;\
				*w->len = kn_hton32(l);\
			}\
			w->writebuf->size += size;\
			return;\
		}

static inline void wpk_write_uint8(wpacket_t w,uint8_t value)
{	
    CHECK_WRITE(uint8_t,value);
    wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_uint16(wpacket_t w,uint16_t value)
{

    value = kn_hton16(value);
    CHECK_WRITE(uint16_t,value);
    wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_uint32(wpacket_t w,uint32_t value)
{
    value = kn_hton32(value);		
    CHECK_WRITE(uint32_t,value);
    wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_uint64(wpacket_t w,uint64_t value)
{
    value = kn_hton64(value);		
    CHECK_WRITE(uint64_t,value);
    wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_ident(wpacket_t w,ident value)
{
    CHECK_WRITE(ident,value);
    wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_double(wpacket_t w ,double value)
{
    CHECK_WRITE(double,value);
	wpk_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpk_write_binary(wpacket_t w,const void *value,uint32_t size)
{
	assert(value);
	wpk_write_uint32(w,size);
	wpk_write(w,(int8_t*)value,size);
}

static inline void wpk_write_string(wpacket_t w ,const char *value)
{
    wpk_write_binary(w,value,strlen(value)+1);
}

void wpk_write_wpk(wpacket_t w,wpacket_t value);

#endif
