#pragma once
#include <immintrin.h>
#include <string>
#include <vector>

// SIMD оптимизированный поиск substring
class SIMDFinder {
public:
    // Boyer-Moore-Horspool с SIMD предпроверкой
    static std::vector<size_t> FindAll(const char* haystack, size_t haystackLen, 
                                       const char* needle, size_t needleLen) {
        std::vector<size_t> results;
        
        if (needleLen == 0 || needleLen > haystackLen) return results;
        
        // Простейший SIMD поиск первого символа
        __m128i needleVec = _mm_set1_epi8(needle[0]);
        size_t simdEnd = haystackLen - 16;
        
        for (size_t i = 0; i <= simdEnd; i += 16) {
            __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(haystack + i));
            __m128i cmp = _mm_cmpeq_epi8(chunk, needleVec);
            int mask = _mm_movemask_epi8(cmp);
            
            while (mask) {
                int pos = __builtin_ctz(mask);
                // Полное сравнение строки
                if (memcmp(haystack + i + pos, needle, needleLen) == 0) {
                    results.push_back(i + pos);
                }
                mask &= mask - 1;
            }
        }
        
        // Оставшиеся байты
        for (size_t i = simdEnd + 1; i <= haystackLen - needleLen; ++i) {
            if (memcmp(haystack + i, needle, needleLen) == 0) {
                results.push_back(i);
            }
        }
        
        return results;
    }
};