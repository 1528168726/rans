//
// Created by alg on 2020/12/08.
//
#include "encoder_and_decoder.h"


size_t encoder_rans(const void *src, size_t src_size, void *dst, size_t dst_max_size) {
    //频率表
    Symbol_stats c_stats(src, src_size);

    //state 范围表
    Range range(c_stats);

    const uint8_t *cur_src = (uint8_t *) src + src_size - 1;
    uint8_t *cur_dst = (uint8_t *) dst;
    uint8_t *dst_limit = ((uint8_t *) dst) + dst_max_size;

    //初始化state
    uint64_t state = range.I_lower_bound();
    for (size_t i = 0; i < src_size; ++i) {
        //make state in the range I_i
        while (state > range.c_upper_bound(*cur_src)) {
            uint8_t out = state % (1 << 8);
            state = state >> 8;

            if (cur_dst >= dst_limit)
                output_error("encoder:: cur dst exceed limit");

            *cur_dst = out;
            ++cur_dst;
        }

        //update state
        state = encoder_update_state(state, *cur_src, c_stats);

        if (state > range.I_upper_bound() || state < range.I_lower_bound())
            output_error("after update, state is not in the range");

        --cur_src;
    }

    //储存最后的状态
    memcpy(cur_dst, &state, sizeof(uint64_t));
    cur_dst += sizeof(uint64_t);
    //储存频率表
    memcpy(cur_dst, c_stats.get_address_of_freqs(), 256 * sizeof(uint16_t));
    //返回数据压缩后大小
    return cur_dst - (uint8_t *) dst + 256 * sizeof(uint16_t);

}


size_t decoder_rans(const void *const src, size_t src_size, void *dst, size_t dst_max_size) {
    uint8_t *dst_cur = (uint8_t *) dst;
    const uint8_t *src_cur = (uint8_t *) src + src_size - 1 - 256 * sizeof(uint16_t);

    //重建频率表
    Symbol_stats c_stats;
    memcpy(c_stats.get_address_of_freqs(), src_cur+1, 256 * sizeof(uint16_t));
    c_stats.rebuilt();

    //建立查找表
    Inv_search_table table(c_stats);

    //重建范围表
    Range range(c_stats);

    //从内存中找到初始状态
    uint64_t state = 0;
    memcpy(&state, src_cur - sizeof(uint64_t)+1, sizeof(uint64_t));
    src_cur -= sizeof(uint64_t);

    while (src_cur >= (uint8_t*)src || state > range.I_lower_bound()) {
        //保证state在I_range里
        while (src_cur - (uint8_t*)src>=0 && state < range.I_lower_bound()) {
            state = state << 8;
            state = state + (uint64_t) (*src_cur);
            --src_cur;
        }
        uint8_t s = decoder_update_state(state, state, c_stats, table);
        *dst_cur = s;
        ++dst_cur;

        if (state < range.c_lower_bound(s) || state > range.c_upper_bound(s))
            output_error("decoding state exceed bound");
        if (dst_cur >= dst_max_size + (uint8_t *) dst)
            output_error("decoder dst size exceed");

    }
    return dst_cur-(uint8_t *)dst;
}

void Symbol_stats::count_freqs(const uint8_t *input, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        ++tmp_freqs[*input];
        ++input;
    }
}

void Symbol_stats::count_cum_freqs() {
    int sum = 0;
    for (int i = 0; i < 256; ++i) {
        cum_freqs[i] = sum;
        sum += freqs[i];
    }
    cum_freqs[256] = sum;
    total = sum;
}

void Symbol_stats::normalize() {
    //向M规整化
    for (int i = 0; i < 257; ++i) {
        cum_freqs[i] = uint64_t(cum_freqs[i]) * M / total;
    }
    total = M;

    //获得新freqs
    for (int i = 0; i < 256; ++i) {
        freqs[i] = cum_freqs[i + 1] - cum_freqs[i];

        //避免文本一直为同一个字符导致算法无法正确运行的问题
        if (freqs[i]==M){
            ++freqs[i-1];
            --freqs[i];
        }
    }

    //如果由于规整化导致个别字符频率由不为0变为0，进行调整
    for (int i = 0; i < 256; ++i) {
        if (tmp_freqs[i] > 0 && freqs[i] == 0) {
            int choice_freq = 0x0fffffff;
            int choice = -1;
            for (int j = 0; j < 256; ++j) {
                if (freqs[j] > 1 && freqs[j] < choice_freq) {
                    choice_freq = freqs[j];
                    choice = j;
                }
            }

            if (choice == -1)
                output_error("Symbol_stats::normalize: choice==-1");

            --freqs[choice];
            ++freqs[i];
        }
    }

    count_cum_freqs();

    if (cum_freqs[256] != M)
        output_error("Symbol_stats::normalize: cum_freqs[256]!=M");
}

Symbol_stats::Symbol_stats(const void *src, size_t src_size) {
    memset(tmp_freqs,0,sizeof(tmp_freqs));
    count_freqs((const uint8_t *) src, src_size);

    //根据tmp_freq刷新cum_freqs
    {
        int sum = 0;
        for (int i = 0; i < 256; ++i) {
            cum_freqs[i] = sum;
            sum += tmp_freqs[i];
        }
        cum_freqs[256] = sum;
        total = sum;
    }
    normalize();
}

void Symbol_stats::rebuilt() {
    count_cum_freqs();

    if (cum_freqs[256] != M)
        output_error("Symbol_stats::rebuilt: rebulit data error");

}

Range::Range(const Symbol_stats &c_stats) {
    limit_range.first = uint64_t(M) << low_bount;
    limit_range.second = uint64_t(M) << (low_bount + 8);
    for (int i = 0; i < 256; ++i) {
        c_range[i].first = uint64_t(c_stats.freqs[i]) << low_bount;
        c_range[i].second = uint64_t(c_stats.freqs[i]) << (low_bount + 8);
    }
}

Inv_search_table::Inv_search_table(Symbol_stats &c_stats) {
    table=new uint8_t [M];
    for (int i = 0; i < 256; ++i) {
        for (int j = c_stats.cum_freqs[i]; j < c_stats.cum_freqs[i+1]; ++j) {
            table[j]= i;
        }
    }
}

Inv_search_table::~Inv_search_table() {
    delete [] table;
}

uint8_t Inv_search_table::search(int slot) {
    return table[slot];
}

uint64_t encoder_update_state(uint64_t state,uint8_t s, const Symbol_stats& c_stats){
    //位移加速
    uint64_t newstate=state/c_stats.freqs[s];
    newstate=newstate<<M_bit;
    newstate+=c_stats.cum_freqs[s]+state%c_stats.freqs[s];
    return newstate;
    //按照公式
//    uint64_t newstate=state/c_stats.freqs[s];
//    newstate*=c_stats.total;
//    newstate+=c_stats.cum_freqs[s];
//    newstate+=state%c_stats.freqs[s];
//    return newstate;
}

uint8_t decoder_update_state(uint64_t state,uint64_t& new_state,const Symbol_stats & c_stats, Inv_search_table& table){
    //位移操作加速运算
    uint64_t tmp_state=state>>M_bit;
    uint64_t slot=state^(tmp_state<<M_bit);
    uint8_t s=table.search(slot);
    tmp_state*=c_stats.freqs[s];
    tmp_state+=slot-c_stats.cum_freqs[s];
    new_state=tmp_state;

    //按照公式
//    uint64_t slot=state%c_stats.total;
//    uint8_t s=table.search(slot);
//    uint64_t tmp_state=state/c_stats.total;
//    tmp_state*=c_stats.freqs[s];
//    tmp_state+=slot;
//    tmp_state-=c_stats.cum_freqs[s];
//    new_state=tmp_state;
    return s;
}

void output_error(std::string out){
    std::cout<<out<<std::endl;
    exit(0);
}