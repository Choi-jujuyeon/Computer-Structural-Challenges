#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 100

// MemoryAccess ����ü ����
// �޸� ���� ������ �����ϴ� ����ü(�۾����� + �޸� �ּ�)
typedef struct {
    char operation;                     // 'l' �Ǵ� 's'�� ����
    unsigned int address;               // 16���� �ּ� ����
} MemoryAccess;

// �� ���� �ؽ�Ʈ�� �޾Ƽ� MemoryAcess ����ü�� ��ȯ
// �Ű����� : line - �Ľ��� ���ڿ�
// ��ȯ �� : �Ľ̵� MemoryAcess ����ü
MemoryAccess parseLine(char* line) {
    MemoryAccess access;                                            // �Ľ̵� ����� ������ ����ü ����
    sscanf(line, " %c %x", &access.operation, &access.address);     // ���ڿ����� �۾� ���� + �ּ� ����
    return access;                                                  // �Ľ̵� ��� ��ȯ
}

// ĳ�� ��� ����ü ����
typedef struct {
    unsigned int tag;      // �޸� �ּ��� ������Ʈ ������ ��� ����
    int valid;             // ��ȿ�ϸ� 1, ��ȿ�ϸ� 0���� ��Ÿ����
} CacheBlock;

//ĳ�� �� ����ü ����
typedef struct {
    CacheBlock* blocks;     // ��ϵ��� �迭
} CacheSet;

//��ü ĳ�ø� ��Ÿ���� ����ü ����
typedef struct {
    CacheSet* sets;         // ĳ���� ��� �� �����ϴ� �迭
    int set_count;          // ĳ�ÿ� ���Ե� ���� �� ����
    int blocks_per_set;     // �� �¿� ���Ե� ����� ��
    int block_size;         // �� ����� ũ��
} Cache;

// ĳ�ø� �ʱ�ȭ�ϴ� �Լ�
Cache initializeCache(int set_count, int blocks_per_set, int block_size) {
    Cache cache;  // Cache ����ü ����

    // Cache ����ü�� �ʵ���� �ʱ�ȭ
    cache.set_count = set_count;                // ���� �� �ʱ�ȭ
    cache.blocks_per_set = blocks_per_set;      // �� �� �� ��� �� �ʱ�ȭ
    cache.block_size = block_size;              // ��� ũ�� �ʱ�ȭ

    // �� �迭�� ���� �Ҵ��Ͽ� �ʱ�ȭ
    cache.sets = (CacheSet*)malloc(set_count * sizeof(CacheSet));

    // �� ���� �ʱ�ȭ
    for (int i = 0; i < set_count; i++) {
        // �� ���� ��� �迭�� ���� �Ҵ��Ͽ� �ʱ�ȭ
        cache.sets[i].blocks = (CacheBlock*)malloc(blocks_per_set * sizeof(CacheBlock));

        // �� ����� �ʱ�ȭ
        for (int j = 0; j < blocks_per_set; j++) {
            cache.sets[i].blocks[j].valid = 0;              // ��ȿ ��Ʈ �ʱ�ȭ (0: ��ȿ)
            cache.sets[i].blocks[j].tag = 0;                // �±� �ʱ�ȭ (0)
        }
    }

    return cache;  // �ʱ�ȭ�� ĳ�� ��ȯ
}

// �־��� �ּҿ��� ĳ�� �� �ε����� ����ϴ� �Լ�
unsigned int getSetIndex(Cache cache, unsigned int address) {
    // �ּҸ� ��� ũ��� ������ ���� ���� ���� �����Ͽ� �� �ε��� ��ȯ
    return (address / cache.block_size) % cache.set_count;
}

// �־��� �ּҿ��� �±׸� ����ϴ� �Լ�
unsigned int getTag(Cache cache, unsigned int address) {
    // �ּҸ� ��� ũ��� ������ ���� ���� ���� �� ��ȯ (���� ��Ʈ ���)
    return (address / cache.block_size) / cache.set_count;
}

// �¿��� �־��� �±׸� ���� ����� ã�� �Լ�
int findBlockIndex(CacheSet set, unsigned int tag, int blocks_per_set) {
    for (int i = 0; i < blocks_per_set; i++) {
        // ��ȿ�ϰ� �±װ� ��ġ�ϴ� ����� ã�� ���
        if (set.blocks[i].valid && set.blocks[i].tag == tag) {
            return i;  // �ش� ����� �ε��� ��ȯ
        }
    }
    return -1;  // ��ġ�ϴ� ����� ������ -1 ��ȯ
}

// ���� �򰡸� ���� ���� ����
int total_loads = 0;
int total_stores = 0;
int load_hits = 0;
int load_misses = 0;
int store_hits = 0;
int store_misses = 0;
int total_cycles = 0;

// ĳ�ÿ� �����ϴ� �Լ� (�ε�/����� ���� ó��)
void accessCache(Cache* cache, char operation, unsigned int address, int write_allocate, int write_through, int eviction_policy) {
    unsigned int set_index = getSetIndex(*cache, address);  // �� �ε��� ���
    unsigned int tag = getTag(*cache, address);  // �±� ���

    CacheSet set = cache->sets[set_index];  // �ش� �� ��������
    int block_index = findBlockIndex(set, tag, cache->blocks_per_set);  // �ش� �¿��� ��� �ε��� ã��

    if (operation == 'l') {                                 // �ε� ������ ���
        total_loads++;                                      // �� �ε� �� ����
        if (block_index != -1) {                            // ĳ�� ��Ʈ�� ���
            load_hits++;                                    // �ε� ��Ʈ �� ����
            total_cycles += 1;                              // ��Ʈ �� ����Ŭ �� ����
            printf("Load Hit\n");                           // �ε� ��Ʈ ���
        }
        else {                                            // ĳ�� �̽��� ���
            load_misses++;                                  // �ε� �̽� �� ����
            total_cycles += 100;                            // �̽� �� ����Ŭ �� ����
            printf("Load Miss\n");                          // �ε� �̽� ���
        }
    }
    else if (operation == 's') {                          // ����� ������ ���
        total_stores++;                                     // �� ����� �� ����
        if (block_index != -1) {                            // ĳ�� ��Ʈ�� ���
            store_hits++;                                   // ����� ��Ʈ �� ����
            total_cycles += 1;                              // ��Ʈ �� ����Ŭ �� ����
            printf("Store Hit\n");                          // ����� ��Ʈ ���
        }
        else {                                            // ĳ�� �̽��� ���
            store_misses++;                                 // ����� �̽� �� ����
            total_cycles += 100;                            // �̽� �� ����Ŭ �� ����
            printf("Store Miss\n");                         // ����� �̽� ���
        }
    }
}

int main(int argc, char* argv[]) {
    // ������ ���� 8���� �ƴϸ� ���� ��� �� ����
    if (argc != 8) {
        fprintf(stderr, "Usage: %s <sets> <blocks_per_set> <block_size> <write_allocate|no-write_allocate> <write_through|write_back> <lru|fifo|random>\n", argv[0]);
        return 1;
    }

    // ����� ���ڸ� ���� ĳ�� ���� �Ķ���� ����
    int set_count = atoi(argv[1]);
    int blocks_per_set = atoi(argv[2]);
    int block_size = atoi(argv[3]);
    int write_allocate = strcmp(argv[4], "write-allocate") == 0;
    int write_through = strcmp(argv[5], "write-through") == 0;
    int eviction_policy = 0; // lru �⺻��
    if (strcmp(argv[6], "fifo") == 0) eviction_policy = 1;
    else if (strcmp(argv[6], "random") == 0) eviction_policy = 2;

    // ĳ�� �ʱ�ȭ
    Cache cache = initializeCache(set_count, blocks_per_set, block_size);
    srand(time(NULL)); // Random eviction�� ���� �õ� ����

    // ǥ�� �Է¿��� �� �پ� �о�ͼ� ĳ�� ���� �Լ� ȣ��
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), stdin)) {
        MemoryAccess access = parseLine(line);
        accessCache(&cache, access.operation, access.address, write_allocate, write_through, eviction_policy);
    }

    // ��� ���
    printf("Total loads: %d\n", total_loads);
    printf("Total stores: %d\n", total_stores);
    printf("Load hits: %d\n", load_hits);
    printf("Load misses: %d\n", load_misses);
    printf("Store hits: %d\n", store_hits);
    printf("Store misses: %d\n", store_misses);
    printf("Total cycles: %d\n", total_cycles);

    // �޸� ����
    for (int i = 0; i < set_count; i++) {
        free(cache.sets[i].blocks);
    }
    free(cache.sets);

    return 0;
}

