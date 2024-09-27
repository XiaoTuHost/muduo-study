#pragma once
#ifndef __BUFFER__H__
#define __BUFFER__H__

#include "nocopyable.h"

#include <vector>
#include <string>

class Buffer : nocopyable{
public:
    static const std::size_t kCheapPrepend = 8;
    static const std::size_t kInitialSize = 1024;

    explicit Buffer(std::size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend+kInitialSize)
        , readIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend)
    {
        
    }

    std::size_t readableBytes() const 
    {
        return writeIndex_ - readIndex_;
    }

    std::size_t writableBytes() const
    {
        return buffer_.size() - writeIndex_;
    }

    std::size_t prependableBytes() const 
    {
        return readIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek () {
        return begin() + readIndex_;
    }

    // onMessage string  <- Buffer
    void retrieve(std::size_t len){
        if(len<readableBytes()){
            readIndex_+=len; //  只读取了可读缓冲区的一部分数据，就是len，还剩下readindex_ +=len
        }else{
            retrieveAll();
        }
    }

    void retrieveAll(){
        readIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 把 onMessage函数上报的buffer数据转换成string类型
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes()); // 应用可读数据
    }

    std::string retrieveAsString(std::size_t len){
        std::string result(peek(),len);
        retrieve(len); // 上面一句把缓冲区中可读数据已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }

    void makeSpace(size_t len){ 
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
            buffer_.resize(writeIndex_+len);
        }else{
            size_t readable = readableBytes();
            std::copy(begin()+readIndex_,begin()+writeIndex_,begin()+kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = kCheapPrepend + readable;
        }   
    }   

    void append(const char * data,size_t len){
        ensureWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        writeIndex_+=len;
    }

    char* beginWrite(){
        return begin()+writeIndex_;
    }

    ssize_t readFd(int fd,int * saveErrno);

private:
    char* begin() {
        return &(*buffer_.begin());
    }

    std::vector<char> buffer_;
    std::size_t readIndex_;
    std::size_t writeIndex_;
};

#endif