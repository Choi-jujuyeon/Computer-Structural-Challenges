#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 100

// MemoryAccess 구조체 정의
// 메모리 접근 정보를 저장하는 구조체(작업종류 + 메모리 주소)
typedef struct {
    char operation;                     // 'l' 또는 's'를 저장
    unsigned int address;               // 16진수 주소 저장
} MemoryAccess;

// 한 줄의 텍스트를 받아서 MemoryAcess 구조체로 변환
// 매개변수 : line - 파싱할 문자열
// 반환 값 : 파싱된 MemoryAcess 구조체
MemoryAccess parseLine(char* line) {
    MemoryAccess access;                                            // 파싱된 결과를 저장할 구조체 정의
    sscanf(line, " %c %x", &access.operation, &access.address);     // 문자열에서 작업 종류 + 주소 추출
    return access;                                                  // 파싱된 결과 반환
}

// 캐시 블록 구조체 정의
typedef struct {
    unsigned int tag;      // 메모리 주소의 상위비트 저장해 블록 구별
    int valid;             // 유효하면 1, 무효하면 0으로 나타내기
} CacheBlock;

//캐시 셋 구조체 정의
typedef struct {
    CacheBlock* blocks;     // 블록들의 배열
} CacheSet;

//전체 캐시를 나타내는 구조체 정의
typedef struct {
    CacheSet* sets;         // 캐시의 모든 셋 저장하는 배열
    int set_count;          // 캐시에 포함된 셋의 수 저장
    int blocks_per_set;     // 각 셋에 포함된 블록의 수
    int block_size;         // 각 블록의 크기
} Cache;

// 캐시를 초기화하는 함수
Cache initializeCache(int set_count, int blocks_per_set, int block_size) {
    Cache cache;  // Cache 구조체 선언

    // Cache 구조체의 필드들을 초기화
    cache.set_count = set_count;                // 셋의 수 초기화
    cache.blocks_per_set = blocks_per_set;      // 각 셋 당 블록 수 초기화
    cache.block_size = block_size;              // 블록 크기 초기화

    // 셋 배열을 동적 할당하여 초기화
    cache.sets = (CacheSet*)malloc(set_count * sizeof(CacheSet));

    // 각 셋을 초기화
    for (int i = 0; i < set_count; i++) {
        // 각 셋의 블록 배열을 동적 할당하여 초기화
        cache.sets[i].blocks = (CacheBlock*)malloc(blocks_per_set * sizeof(CacheBlock));

        // 각 블록을 초기화
        for (int j = 0; j < blocks_per_set; j++) {
            cache.sets[i].blocks[j].valid = 0;              // 유효 비트 초기화 (0: 무효)
            cache.sets[i].blocks[j].tag = 0;                // 태그 초기화 (0)
        }
    }

    return cache;  // 초기화된 캐시 반환
}

// 주어진 주소에서 캐시 셋 인덱스를 계산하는 함수
unsigned int getSetIndex(Cache cache, unsigned int address) {
    // 주소를 블록 크기로 나누고 셋의 수로 모듈로 연산하여 셋 인덱스 반환
    return (address / cache.block_size) % cache.set_count;
}

// 주어진 주소에서 태그를 계산하는 함수
unsigned int getTag(Cache cache, unsigned int address) {
    // 주소를 블록 크기로 나누고 셋의 수로 나눈 값 반환 (상위 비트 사용)
    return (address / cache.block_size) / cache.set_count;
}

// 셋에서 주어진 태그를 가진 블록을 찾는 함수
int findBlockIndex(CacheSet set, unsigned int tag, int blocks_per_set) {
    for (int i = 0; i < blocks_per_set; i++) {
        // 유효하고 태그가 일치하는 블록을 찾을 경우
        if (set.blocks[i].valid && set.blocks[i].tag == tag) {
            return i;  // 해당 블록의 인덱스 반환
        }
    }
    return -1;  // 일치하는 블록이 없으면 -1 반환
}

// 성능 평가를 위한 전역 변수
int total_loads = 0;
int total_stores = 0;
int load_hits = 0;
int load_misses = 0;
int store_hits = 0;
int store_misses = 0;
int total_cycles = 0;

// 캐시에 접근하는 함수 (로드/스토어 연산 처리)
void accessCache(Cache* cache, char operation, unsigned int address, int write_allocate, int write_through, int eviction_policy) {
    unsigned int set_index = getSetIndex(*cache, address);  // 셋 인덱스 계산
    unsigned int tag = getTag(*cache, address);  // 태그 계산

    CacheSet set = cache->sets[set_index];  // 해당 셋 가져오기
    int block_index = findBlockIndex(set, tag, cache->blocks_per_set);  // 해당 셋에서 블록 인덱스 찾기

    if (operation == 'l') {                                 // 로드 연산인 경우
        total_loads++;                                      // 총 로드 수 증가
        if (block_index != -1) {                            // 캐시 히트인 경우
            load_hits++;                                    // 로드 히트 수 증가
            total_cycles += 1;                              // 히트 시 사이클 수 증가
            printf("Load Hit\n");                           // 로드 히트 출력
        }
        else {                                            // 캐시 미스인 경우
            load_misses++;                                  // 로드 미스 수 증가
            total_cycles += 100;                            // 미스 시 사이클 수 증가
            printf("Load Miss\n");                          // 로드 미스 출력
        }
    }
    else if (operation == 's') {                          // 스토어 연산인 경우
        total_stores++;                                     // 총 스토어 수 증가
        if (block_index != -1) {                            // 캐시 히트인 경우
            store_hits++;                                   // 스토어 히트 수 증가
            total_cycles += 1;                              // 히트 시 사이클 수 증가
            printf("Store Hit\n");                          // 스토어 히트 출력
        }
        else {                                            // 캐시 미스인 경우
            store_misses++;                                 // 스토어 미스 수 증가
            total_cycles += 100;                            // 미스 시 사이클 수 증가
            printf("Store Miss\n");                         // 스토어 미스 출력
        }
    }
}

int main(int argc, char* argv[]) {
    // 인자의 수가 8개가 아니면 사용법 출력 후 종료
    if (argc != 8) {
        fprintf(stderr, "Usage: %s <sets> <blocks_per_set> <block_size> <write_allocate|no-write_allocate> <write_through|write_back> <lru|fifo|random>\n", argv[0]);
        return 1;
    }

    // 명령줄 인자를 통해 캐시 구성 파라미터 설정
    int set_count = atoi(argv[1]);
    int blocks_per_set = atoi(argv[2]);
    int block_size = atoi(argv[3]);
    int write_allocate = strcmp(argv[4], "write-allocate") == 0;
    int write_through = strcmp(argv[5], "write-through") == 0;
    int eviction_policy = 0; // lru 기본값
    if (strcmp(argv[6], "fifo") == 0) eviction_policy = 1;
    else if (strcmp(argv[6], "random") == 0) eviction_policy = 2;

    // 캐시 초기화
    Cache cache = initializeCache(set_count, blocks_per_set, block_size);
    srand(time(NULL)); // Random eviction을 위한 시드 설정

    // 표준 입력에서 한 줄씩 읽어와서 캐시 접근 함수 호출
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), stdin)) {
        MemoryAccess access = parseLine(line);
        accessCache(&cache, access.operation, access.address, write_allocate, write_through, eviction_policy);
    }

    // 결과 출력
    printf("Total loads: %d\n", total_loads);
    printf("Total stores: %d\n", total_stores);
    printf("Load hits: %d\n", load_hits);
    printf("Load misses: %d\n", load_misses);
    printf("Store hits: %d\n", store_hits);
    printf("Store misses: %d\n", store_misses);
    printf("Total cycles: %d\n", total_cycles);

    // 메모리 해제
    for (int i = 0; i < set_count; i++) {
        free(cache.sets[i].blocks);
    }
    free(cache.sets);

    return 0;
}

