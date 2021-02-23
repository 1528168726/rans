//
// Created by alg on 2020/12/08.
//
#include <cstdio>
#include <cstdint>
#include <utility>
#include <string>
#include <cstring>
#include <iostream>

#ifndef RANS_ENCODER_AND_DECODER_H
#define RANS_ENCODER_AND_DECODER_H

//压缩接口
//src：压缩文件的内存地址
//src_size：压缩内容的大小，单位：byte
//dst：存放压缩后内容的内存地址
//dst_size：存放压缩后内容内存的最大值
//返回压缩后大小
size_t encoder_rans(const void *src, size_t src_size, void *dst, size_t dst_max_size);

//解压接口
//src：待解压文件的内存地址
//src_size：待解压内容的大小，单位：byte
//dst：存放解压内容的内存地址
//dst_size：存放解压内容内存的最大值
//返回解压后大小
size_t decoder_rans(const void *src, size_t src_size, void *dst, size_t dst_max_size);

//设置I = [2^low_bound*M,2^8*2^low_bound*M-1]
//设置I_i = [2^low_bound*freq,2^8*2^low_bound*freq-1]
#define low_bount 40
#define M (1<<14)
#define M_bit 14

//频率表
class Symbol_stats {
public:
    //压缩后储存的频率表为freq
    uint16_t freqs[256];
    uint32_t tmp_freqs[256];
    uint32_t cum_freqs[257];
    uint32_t total;

    void count_freqs(const uint8_t *input, size_t size);
    void count_cum_freqs();
    void normalize();

    Symbol_stats(const void *src, size_t src_size);
    Symbol_stats(){memset(freqs,0,sizeof(freqs));total=0;}
    void rebuilt();
    void* get_address_of_freqs(){return (void*)&freqs;}
};

//state的范围
class Range {
    std::pair<uint64_t, uint64_t> c_range[256];
    std::pair<uint64_t, uint64_t> limit_range;

public:
    Range(const Symbol_stats& c_stats);
    uint64_t I_lower_bound() const{return limit_range.first;}
    uint64_t I_upper_bound() const{return limit_range.second;}
    uint64_t c_lower_bound(int i){return c_range[i].first;}
    uint64_t c_upper_bound(int i){return c_range[i].second;}

};

//解压时的slot查找表
class Inv_search_table {
    uint8_t *table;
public:
    Inv_search_table(Symbol_stats &c_stats);

    ~Inv_search_table();

    //返回解压后的字符s
    uint8_t search(int slot);
};

//压缩时更新state的函数，按照公式
uint64_t encoder_update_state(uint64_t state,uint8_t s, const Symbol_stats& c_stats);

//解压时更新state的函数，按照公式
uint8_t decoder_update_state(uint64_t state,uint64_t& new_state,const Symbol_stats & c_stats, Inv_search_table& table);

void output_error(std::string out);

#endif //RANS_ENCODER_AND_DECODER_H

