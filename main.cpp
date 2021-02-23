#include <iostream>
#include "lib/encoder_and_decoder.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace std;

//读文件
void *read_file(char const *filename, std::size_t &out_size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        output_error("file not found");
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *buf = new uint8_t[size + 1000];
    out_size = fread(buf, 1, size + 1000, f);
    fclose(f);


    if (out_size <= 0)
        output_error("wroing input");

//    if(out_size==size)
//        cout<<"hhhyes\n";
    return buf;
}

//写入文件
std::size_t write_file(const char *filename, void *output, std::size_t size) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        output_error("file open error");
    }

    std::size_t output_len = fwrite(output, 1, size, f);
    fclose(f);
    return output_len;
}

int body(int argc,char const *argv[]){
    //无命令
    if (argc == 1) {
        printf("no order\n\"ziptxt\"to zip,\"unziptxt\" to unzip\n");
        return 0;
    }

    //命令为 “ziptxt”
    if (strcmp(argv[1], "ziptxt") == 0) {
        if (argc == 2) {
            printf("no input file\n");
            return 0;
        }
        if (argc > 4) {
            printf("to many argument\nziptxt [source_file] [des_file]\n");
        }
        const char *output;
        //若无第四个参数，设置默认文件输出名
        if (argc == 3)
            output = "default_ziped.txt";
        else
            output = argv[3];

        size_t input_len;
        void *buffer_in = read_file(argv[2], input_len);
        void *buffer_out = new uint8_t[input_len + 1024];
        printf("input size:%llu bytes\n", input_len);

        //---------------zip_fun-----------------------------------------------------------

        size_t output_len = encoder_rans(buffer_in, input_len, buffer_out, input_len + 1024);

        //----------------------------------------------------------------------------------
        auto finish_len = write_file(output, buffer_out, output_len);

        if (finish_len == output_len) {
            printf("succeed! output file name: %s \noutput %llu bytes\nzip rate = %.2lf%%\n", output, finish_len,
                   ((double) finish_len) / input_len * 100);
            return 0;
        } else {
            printf("fail write! zip_len=%llu bytes, writen_len =%llu bytes\n", output_len, finish_len);
            return 0;
        }
    }

        //命令为 "unziptxt"
    else if (strcmp(argv[1], "unziptxt") == 0) {
        if (argc == 2) {
            printf("no input file\n");
            return 0;
        }
        if (argc > 4) {
            printf("to many argument\nunziptxt [source_file] [des_file]\n");
        }
        const char *output;
        //若无第四个参数，设置默认文件输出名
        if (argc == 3)
            output = "default_unzip.txt";
        else
            output = argv[3];

        size_t input_len;
        void *buffer_in = read_file(argv[2], input_len);
        void *buffer_out = new uint8_t[10 * input_len + size_t(6e7)];
        //-------------------unzip_fun-------------------------------------------------

        size_t output_len = decoder_rans(buffer_in, input_len, buffer_out, 10 * input_len + size_t(6e7));

        //-----------------------------------------------------------------------------

        auto finish_len = write_file(output, buffer_out, output_len);

        if (finish_len == output_len) {
            printf("succeed! output file name: %s \noutput %llu bytes\nzip rate = %.2lf%%\n", output, finish_len,
                   ((double) finish_len) / input_len * 100);
            return 0;
        } else {
            printf("fail write! unzip_len=%llu bytes, writen_len =%llu bytes\n", output_len, finish_len);
            return 0;
        }
    }

        //命令错误
    else {
        printf("wrong order\n\"ziptxt\"to zip,\"unziptxt\" to unzip\n");
        return 0;
    }
}

int main(int argc, char const *argv[]) {
    body(argc,argv);

//    char a[]="1111";
//    char buff[10000];
//    char buff1[10000];
//    std::size_t len=encoder_rans(a,sizeof(a)-1,buff,10000);
//    std::size_t len2=decoder_rans(buff,len,buff1,10000);
//    buff1[len2]=0;
//    cout<<"len1: "<<len<<"  len2: "<<len2<<endl;
//    cout<<buff1;
}