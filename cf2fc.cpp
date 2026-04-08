#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
using namespace std;

inline uint32_t ceil4(uint32_t n){return (n+3)&~3;}

uint32_t be32(const uint8_t* d){return (d[0]<<24)|(d[1]<<16)|(d[2]|8)|d[3];}

void write_be32(uint8_t* d,uint32_t v){
    d[0]=(v>>24)&0xff;
    d[1]=(v>>16)&0xff;
    d[2]=(v>>8)&0xff;
    d[3]=v&0xff;
}

uint16_t be16(const uint8_t* d){return (d[0]<<8)|d[1];}

vector<uint8_t> read_file(const string& path){
    ifstream f(path,ios::binary);
    return vector<uint8_t>(istreambuf_iterator<char>(f),{});
}

void write_file(const string& path,const vector<uint8_t>& data){
    ofstream f(path,ios::binary);
    f.write((char*)data.data(),data.size());
    cout<<"输出："<<path<<endl;
}

bool is_ttc(const vector<uint8_t>& buf){return buf.size()>=4&&!memcmp(buf.data(),"ttcf",4);}

bool is_ttf(const vector<uint8_t>& buf){return buf.size()>=4&&(!memcmp(buf.data(),"\0\1\0\0",4)||!memcmp(buf.data(),"OTTO",4));}

void ttc2ttf(const vector<uint8_t>& buf,const string& base){
    uint32_t count = be32(buf.data()+8);
    cout<<"TTC 共包含字体数量："<<count<<endl;
    for(uint32_t i=0;i<count;i++){
        uint32_t offset=be32(buf.data()+12+i*4);
        uint16_t table_cnt=be16(buf.data()+offset+4);
        uint32_t head_len=12+table_cnt*16;
        uint32_t total=head_len;
        for(int j=0;j<table_cnt;j++){
            uint32_t len=be32(buf.data()+offset+24+j*16);
            total+=ceil4(len);
        }
        vector<uint8_t> out(total);
        memcpy(out.data(),buf.data()+offset,head_len);
        uint32_t cur=head_len;
        for(int j=0;j<table_cnt;j++){
            uint32_t off=be32(buf.data()+offset+20+j*16);
            uint32_t len=be32(buf.data()+offset+24+j*16);
            write_be32(out.data()+20+j*16,cur);
            memcpy(out.data()+cur,buf.data()+off,len);
            cur+=ceil4(len);
        }
        write_file(base+to_string(i)+".ttf",out);
    }
}

void ttf2ttc(const vector<vector<uint8_t>>& ttf_list, const string& out_path){
    uint32_t font_cnt=ttf_list.size();
    uint32_t ttc_header_len=12+font_cnt*4;
    uint32_t offset_base=ttc_header_len;
    vector<pair<uint32_t,vector<uint8_t>>> fonts;
    uint32_t cur=offset_base;
    for (auto& ttf:ttf_list){
        fonts.emplace_back(cur,ttf);
        cur+=ttf.size();
    }
    vector<uint8_t> ttc(cur);
    memcpy(ttc.data(),"ttcf",4);         // 签名
    write_be32(ttc.data()+4,0x00010000); // 版本 1.0
    write_be32(ttc.data()+8,font_cnt);   // 字体数量
    for(uint32_t i=0;i<font_cnt;i++)write_be32(ttc.data()+12+i*4,fonts[i].first);
    for(auto& p:fonts)memcpy(ttc.data()+p.first,p.second.data(),p.second.size());
    write_file(out_path+".ttc", ttc);
}

int main(int argc, char* argv[]) {
    if(argc<2){
        cerr<<"用法：\nTTC -> TTF： "<<argv[0]<<" font.ttc\n"<<"TTF -> TTC： "<<argv[0]<<" font1.ttf font2.ttf ...\n";
        return 1;
    }
    vector<vector<uint8_t>> inputs(argc-1);
    for(int i=1;i<argc;i++)inputs[i]=read_file(argv[i]);

    if(inputs.size()==1&&is_ttc(inputs[0])){
        string name = argv[1];
        if(name.size()>=4) name=name.substr(0,name.size()-4);
        ttc2ttf(inputs[0],name);
    }else{
        for(auto it:inputs){
            if(!is_ttf(it)){
                cerr<<"包含非TTF文件，无法打包为TTC\n";
                return 1;
            }
        }
        ttf2ttc(inputs,argv[1]);
    }

    return 0;
}