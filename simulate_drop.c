#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* 原有的多次遍历版本 */
void simulate_drop_multi(int* matrix, int width, int height) {
    bool changed = true;
    while (changed) {
        changed = false;
        // 从左下到右上遍历
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                int idx = col * height + row;
                int val = matrix[idx];
                // 可移动物体(>1)
                if (val > 1) {
                    // 下方
                    if (row > 0) {
                        int down_idx = col * height + (row - 1);
                        if (matrix[down_idx] == 0) {
                            matrix[down_idx] = val;
                            matrix[idx] = 0;
                            changed = true;
                            continue;
                        }
                        // 左下
                        if (col > 0) {
                            int left_down_idx = (col - 1) * height + (row - 1);
                            if (matrix[left_down_idx] == 0) {
                                matrix[left_down_idx] = val;
                                matrix[idx] = 0;
                                changed = true;
                                continue;
                            }
                        }
                        // 右下
                        if (col < width - 1) {
                            int right_down_idx = (col + 1) * height + (row - 1);
                            if (matrix[right_down_idx] == 0) {
                                matrix[right_down_idx] = val;
                                matrix[idx] = 0;
                                changed = true;
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* 单次遍历版本 */
void simulate_drop_single(int* matrix, int width, int height) {
    // 从左下到右上遍历
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int idx = col * height + row;
            int val = matrix[idx];
            if (val > 1) {
                // 先清空当前位置
                matrix[idx] = 0;

                int cur_row = row;
                int cur_col = col;

                while (true) {
                    if (cur_row > 0) {
                        int down_idx = cur_col * height + (cur_row - 1);
                        if (matrix[down_idx] == 0) {
                            cur_row--;
                            continue;
                        }
                        // 左下
                        if (cur_col > 0) {
                            int left_down_idx = (cur_col - 1) * height + (cur_row - 1);
                            if (matrix[left_down_idx] == 0) {
                                cur_col--;
                                cur_row--;
                                continue;
                            }
                        }
                        // 右下
                        if (cur_col < width - 1) {
                            int right_down_idx = (cur_col + 1) * height + (cur_row - 1);
                            if (matrix[right_down_idx] == 0) {
                                cur_col++;
                                cur_row--;
                                continue;
                            }
                        }
                    }
                    break;
                }

                // 放到最终位置
                int final_idx = cur_col * height + cur_row;
                matrix[final_idx] = val;
            }
        }
    }
}

/* 定义一个表示格子坐标的结构体 */
typedef struct {
    int col;
    int row;
} Cell;

// 比较函数：按照 row 升序（row 越小在下方越先处理），当 row 相同则按 col 升序（col 越小在左侧越先处理）
int compare_cells(const void* a, const void* b) {
    const Cell* ca = (const Cell*) a;
    const Cell* cb = (const Cell*) b;
    if (ca->row != cb->row)
        return ca->row - cb->row;
    return ca->col - cb->col;
}

/*
 * 使用稳定标记与依赖传播的掉落模拟函数。
 *
 * 掉落规则：
 *   - 盘面以二维数组表示，下标规则：index = col * height + row, row=0在最下方。
 *   - 对于可移动物体（值>1），若下方为空，则向下移动；若下方不空，则依次检查左下、右下。
 *   - 每个物体只移动一步，重复此过程直到整个盘面稳定。
 *
 * 优化思路：
 *   1. 为每个格子维护一个稳定标记（stable）：如果该格子的下方、左下和右下都不为空（且为障碍或稳定物体），则认为它稳定，
 *      此后只要它下方支撑不发生变化，就不必再检查。
 *   2. 依赖传播：当一个格子移动后，其原来的位置变为空，这会使得其上方邻域（左上、正上、右上）可能失去支撑，
 *      只有当这些支撑发生变化时，才将相关上方格子从稳定状态撤销，并重新加入待检查列表。
 *   3. 如果一个格子检查后发现支撑条件不满足（存在空位），则保持不稳定并加入下一轮 active 列表；如果支撑完全满足，
 *      则标记为稳定，不再重复检查，直到上方依赖发生变化时被激活。
 */
void simulate_drop_active(int* matrix, int width, int height) {
    int capacity = width * height;
    
    // 为每个格子分配稳定标记数组，初始均为 false
    bool* stable = (bool*) malloc(sizeof(bool) * capacity);
    for (int i = 0; i < capacity; i++) {
        stable[i] = false;
    }
    
    // 初始化 active 列表：将所有可移动物体的位置加入其中
    Cell* active = (Cell*) malloc(sizeof(Cell) * capacity);
    int active_count = 0;
    for (int col = 0; col < width; col++) {
        for (int row = 0; row < height; row++) {
            int idx = col * height + row;
            if (matrix[idx] > 1) {
                active[active_count].col = col;
                active[active_count].row = row;
                active_count++;
            }
        }
    }
    
    while (active_count > 0) {
        // 排序 active 列表，保证从左下到右上（row 从小到大，col 从小到大）
        qsort(active, active_count, sizeof(Cell), compare_cells);
        
        // 新一轮 active 列表
        Cell* new_active = (Cell*) malloc(sizeof(Cell) * capacity);
        int new_active_count = 0;
        
        // 标记本轮是否有物体移动
        bool any_moved = false;
        
        for (int i = 0; i < active_count; i++) {
            int col = active[i].col;
            int row = active[i].row;
            int idx = col * height + row;
            
            // 如果该位置已不含可移动物体，则跳过
            if (matrix[idx] <= 1)
                continue;
            
            // 如果该格子已经稳定，则不需要重新检查
            if (stable[idx])
                continue;
            
            bool canMove = false;
            int dest_col = col, dest_row = row;
            // 检查正下方：只有 row>0 才有下方
            if (row > 0) {
                int down_idx = col * height + (row - 1);
                if (matrix[down_idx] == 0) {
                    dest_row = row - 1;
                    canMove = true;
                }
                // 若正下方不空，检查左下
                else if (col > 0) {
                    int left_down_idx = (col - 1) * height + (row - 1);
                    if (matrix[left_down_idx] == 0) {
                        dest_col = col - 1;
                        dest_row = row - 1;
                        canMove = true;
                    }
                }
                // 若左下也不空，检查右下
                if (!canMove && col < width - 1) {
                    int right_down_idx = (col + 1) * height + (row - 1);
                    if (matrix[right_down_idx] == 0) {
                        dest_col = col + 1;
                        dest_row = row - 1;
                        canMove = true;
                    }
                }
            }
            
            if (canMove) {
                // 移动：目标位置赋值，当前格子置空
                int dest_idx = dest_col * height + dest_row;
                matrix[dest_idx] = matrix[idx];
                matrix[idx] = 0;
                any_moved = true;
                
                // 目标格子必然不是稳定的
                stable[dest_idx] = false;
                // 将目标格子加入下一轮 active 列表
                new_active[new_active_count].col = dest_col;
                new_active[new_active_count].row = dest_row;
                new_active_count++;
                
                // 依赖传播：原位置变为空后，上方邻域可能失去支撑。
                // 我们将左上、正上、右上（如果在边界内）检查，如果这些格子中有可移动物体且已标记为稳定，则撤销稳定并加入 active。
                if (row + 1 < height) {
                    // 正上
                    int up_idx = col * height + (row + 1);
                    if (matrix[up_idx] > 1 && stable[up_idx]) {
                        stable[up_idx] = false;
                        new_active[new_active_count].col = col;
                        new_active[new_active_count].row = row + 1;
                        new_active_count++;
                    }
                    // 左上
                    if (col > 0) {
                        int left_up_idx = (col - 1) * height + (row + 1);
                        if (matrix[left_up_idx] > 1 && stable[left_up_idx]) {
                            stable[left_up_idx] = false;
                            new_active[new_active_count].col = col - 1;
                            new_active[new_active_count].row = row + 1;
                            new_active_count++;
                        }
                    }
                    // 右上
                    if (col < width - 1) {
                        int right_up_idx = (col + 1) * height + (row + 1);
                        if (matrix[right_up_idx] > 1 && stable[right_up_idx]) {
                            stable[right_up_idx] = false;
                            new_active[new_active_count].col = col + 1;
                            new_active[new_active_count].row = row + 1;
                            new_active_count++;
                        }
                    }
                }
            }
            else {
                // 如果不能移动，则检查其支撑情况
                bool support_empty = false;
                if (row > 0) {
                    int down_idx = col * height + (row - 1);
                    if (matrix[down_idx] == 0)
                        support_empty = true;
                    else if (col > 0) {
                        int left_down_idx = (col - 1) * height + (row - 1);
                        if (matrix[left_down_idx] == 0)
                            support_empty = true;
                    }
                    else if (col < width - 1) {
                        int right_down_idx = (col + 1) * height + (row - 1);
                        if (matrix[right_down_idx] == 0)
                            support_empty = true;
                    }
                }
                if (!support_empty) {
                    // 如果下方、左下、右下都不为空，则该格子处于支撑完备状态，标记为稳定
                    stable[idx] = true;
                    // 不加入下一轮 active 列表（因为不可能移动，且支撑未改变）
                }
                else {
                    // 支撑存在空位，但本轮没有移动，则保留该格子为待检查状态
                    // 注意：此时不修改 stable 标记（保持为 false），以便后续检测支撑变化时能触发移动
                    new_active[new_active_count].col = col;
                    new_active[new_active_count].row = row;
                    new_active_count++;
                }
            }
        }
        
        // 释放上一轮 active 列表，更新 active 为新生成的列表
        free(active);
        active = new_active;
        active_count = new_active_count;
        
        // 如果本轮没有任何移动，则说明所有待检查物体都处于稳定状态，可以退出循环
        if (!any_moved)
            break;
    }
    
    free(active);
    free(stable);
}

/* 队列驱动（事件驱动）版本  */

/* 优先级队列结构体 */
typedef struct {
    Cell* data;
    int capacity;
    int size;
    int width; // 用于计算优先级的辅助参数
} PriorityQueue;

/* 计算一个格子的优先级，优先级公式：priority = row * width + col */
int cell_priority(const Cell* cell, int width) {
    return cell->row * width + cell->col;
}

/* 初始化优先级队列 */
void pq_init(PriorityQueue* pq, int capacity, int width) {
    pq->data = (Cell*) malloc(sizeof(Cell) * capacity);
    pq->capacity = capacity;
    pq->size = 0;
    pq->width = width;
}

/* 交换两个格子 */
void pq_swap(Cell* a, Cell* b) {
    Cell temp = *a;
    *a = *b;
    *b = temp;
}

/* 入队操作：将一个格子加入优先级队列 */
void pq_push(PriorityQueue* pq, Cell cell) {
    if (pq->size >= pq->capacity) {
        pq->capacity *= 2;
        pq->data = realloc(pq->data, sizeof(Cell) * pq->capacity);
    }
    int i = pq->size;
    pq->data[i] = cell;
    pq->size++;
    // 向上调整，保持最小堆性质
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (cell_priority(&pq->data[i], pq->width) < cell_priority(&pq->data[parent], pq->width)) {
            pq_swap(&pq->data[i], &pq->data[parent]);
            i = parent;
        } else {
            break;
        }
    }
}

/* 判断队列是否为空 */
bool pq_empty(PriorityQueue* pq) {
    return pq->size == 0;
}

/* 出队操作：取出优先级最低的元素 */
Cell pq_pop(PriorityQueue* pq) {
    Cell top = pq->data[0];
    pq->size--;
    pq->data[0] = pq->data[pq->size];
    int i = 0;
    // 向下调整，保持最小堆性质
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left < pq->size && cell_priority(&pq->data[left], pq->width) < cell_priority(&pq->data[smallest], pq->width))
            smallest = left;
        if (right < pq->size && cell_priority(&pq->data[right], pq->width) < cell_priority(&pq->data[smallest], pq->width))
            smallest = right;
        if (smallest != i) {
            pq_swap(&pq->data[i], &pq->data[smallest]);
            i = smallest;
        } else {
            break;
        }
    }
    return top;
}

/* 释放优先级队列 */
void pq_free(PriorityQueue* pq) {
    free(pq->data);
}

/* 队列驱动（事件驱动）的掉落模拟函数，使用优先级队列保证顺序为从左下到右上 */
void simulate_drop_queue(int* matrix, int width, int height) {
    int initCapacity = width * height;
    PriorityQueue pq;
    pq_init(&pq, initCapacity, width);

    // 初始化队列：将所有可移动物体（值>1）的位置入队
    for (int col = 0; col < width; col++) {
        for (int row = 0; row < height; row++) {
            int idx = col * height + row;
            if (matrix[idx] > 1) {
                Cell cell = {col, row};
                pq_push(&pq, cell);
            }
        }
    }

    while (!pq_empty(&pq)) {
        // 始终取出优先级最低（也就是最靠下且最靠左）的元素
        Cell cur = pq_pop(&pq);
        int col = cur.col;
        int row = cur.row;
        int idx = col * height + row;
        int val = matrix[idx];

        // 如果该位置已经不含可移动物体，则跳过
        if (val <= 1)
            continue;

        bool moved = false;
        int new_col = col, new_row = row;
        if (row > 0) {
            int down_idx = col * height + (row - 1);
            if (matrix[down_idx] == 0) {
                new_row = row - 1;
                moved = true;
            }
            else if (col > 0) {
                int left_down_idx = (col - 1) * height + (row - 1);
                if (matrix[left_down_idx] == 0) {
                    new_col = col - 1;
                    new_row = row - 1;
                    moved = true;
                }
            }
            if (!moved && col < width - 1) {
                int right_down_idx = (col + 1) * height + (row - 1);
                if (matrix[right_down_idx] == 0) {
                    new_col = col + 1;
                    new_row = row - 1;
                    moved = true;
                }
            }
        }

        if (moved) {
            // 执行移动操作
            matrix[new_col * height + new_row] = val;
            matrix[idx] = 0;

            // 将新位置入队（继续处理其下落）
            Cell new_cell = {new_col, new_row};
            pq_push(&pq, new_cell);

            // 原位置上方的格子也可能受到影响，入队待处理
            if (row + 1 < height) {
                Cell above = {col, row + 1};
                pq_push(&pq, above);
            }
        }
    }
    pq_free(&pq);
}