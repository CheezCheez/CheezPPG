/**
 * @file RingBuffer.h
 * @author Vecang @Cheez
 * @brief 
 * @version 0.1
 * @date 2025-04-29
 * 
 * @copyright Copyright (c) 2025
******************************************************************************
* @attention
*
* Copyright (c) 2024 Cheez.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/
#ifndef CIRCULAR_BUFFER_H  
#define CIRCULAR_BUFFER_H  

template <typename T, int SIZE>  
class RingBuffer {  
private:  
    T buffer[SIZE];  
    int head;  
    int tail;  
    int count;  

public:  
    // 构造函数  
    RingBuffer() : head(0), tail(0), count(0) {}  

    // 初始化缓冲区  
    void init() {  
        head = 0;  
        tail = 0;  
        count = 0;  
    }  

    // 检查缓冲区是否为空  
    bool isEmpty() const {  
        return count == 0;  
    }  

    // 检查缓冲区是否已满  
    bool isFull() const {  
        return count == SIZE;  
    }  

    // 向缓冲区添加元素  
    bool push(const T& value) {  
        if (isFull()) {  
            // 如果缓冲区已满，覆盖最旧的元素  
            head = (head + 1) % SIZE;  
            count--;  
        }  
        
        buffer[tail] = value;  
        tail = (tail + 1) % SIZE;  
        count++;  
        
        return true;  
    }  

    // 从缓冲区移除元素  
    bool pop(T& value) {  
        if (isEmpty()) {  
            return false;  
        }  
        
        value = buffer[head];  
        head = (head + 1) % SIZE;  
        count--;  
        
        return true;  
    }  
    
    // 简单移除元素但不返回值  
    bool pop() {  
        if (isEmpty()) {  
            return false;  
        }  
        
        head = (head + 1) % SIZE;  
        count--;  
        
        return true;  
    }  

    // 查看指定索引的元素，不移除  
    bool peek(int index, T& value) const {  
        if (isEmpty() || index < 0 || index >= count) {  
            return false;  
        }  

        int pos = (head + index) % SIZE;  
        value = buffer[pos];  
        return true;  
    }  
    
    // 获取缓冲区中的元素数量  
    int numItems() const {  
        return count;  
    }  
    
    // 获取缓冲区容量  
    int size() const {  
        return SIZE;  
    }  
};  

#endif // CIRCULAR_BUFFER_H  