# Kiwi 数据科学家校招笔试题 —— 模拟掉落

## 项目简介

本项目实现了一个模拟物体“掉落”的算法，用以模拟二维游戏盘面中物体的重力下落过程。盘面使用一个二维矩阵表示，其中：
- `0` 表示空位，
- `1` 表示固定障碍（不可移动），
- 大于 `1` 的正整数表示可移动物体。

物体的掉落规则如下：
1. 下落规则：  
   - 当物体正下方为空时，下落一格；
   - 如果正下方不为空，则检查左下方是否为空，若为空则侧滑一格；
   - 如果左下也不为空，则检查右下方是否为空，若为空则侧滑一格；
2. 重复上述过程，直到物体无法移动为止；  
3. 对所有物体重复执行上述操作，直至整个盘面稳定。

## 实现方法

为了全面考察工程实现能力，项目中实现了四种不同的算法版本，每种版本都编译为一个动态库供外部调用：

1. #### **simulate_drop_multi**  
   
   采用全盘多次遍历的方式，直至盘面稳定。  
   **思路**：  
   
   ```pseudo
   changed = true
   while changed:
       changed = false
       for each cell from left-bottom to right-top:
           if cell contains movable object:
               if cell below is empty:
                   move object one step down; changed = true
               else if left-down is empty:
                   move object one step left-down; changed = true
               else if right-down is empty:
                   move object one step right-down; changed = true
   ```
   每次轮询都会扫描整个矩阵。
   
2. #### **simulate_drop_single**  
   
   对每个物体直接执行“到底”的移动，即从当前格子一路下落到最终停靠位置。  
   **思路**：  
   
   ```pseudo
   for each cell from left-bottom to right-top:
       if cell contains movable object:
           clear current cell
           while movement possible:
               if cell below is empty: move down
               else if left-down is empty: move left-down
               else if right-down is empty: move right-down
           set final position to object
   ```
   此方法在一次遍历中直接获得最终位置。
   
3. #### **simulate_drop_queue**  
   
   采用队列驱动（事件驱动）的方式：  
   **思路**：  
   
   ```pseudo
   initialize queue with all movable objects
   while queue not empty:
       cell = dequeue
       if object in cell can move (check down, then left-down, then right-down):
           move object one step
           enqueue destination cell and affected neighbor(s)
   ```
   这种方法只处理“活跃区域”，但其更新顺序依赖队列顺序，可能导致与全盘扫描的结果不一致。
   
4. #### **simulate_drop_active (本次重点改进版)**  
   
   使用“稳定标记与依赖传播”的策略，避免对已经稳定的区域进行重复检查，同时仅在支撑条件（下方区域）变化时重新激活上方受影响的格子。  
   **核心思路**：
   
   - 为每个格子维护一个 `stable` 标记。如果一个物体的下方、左下、右下都不为空（即为障碍或者已稳定的物体），则标记为稳定，不再重复检查；
   - 当一个物体移动后，其原位置变为空，则传播依赖：仅当支撑发生变化时，将其上方邻域（左上、正上、右上）标记为“不稳定”，重新加入待检查队列；
   - 一旦一个格子稳定，且其下方支撑没有变化，则不会被重新激活，从而避免了冗余的全盘扫描。
   
   **伪代码**：
   ```pseudo
   Initialize stable[] to false for all cells
   Build active list with all movable objects
   
   while active list is not empty:
       sort active list (from left-bottom to right-top)
       new_active = empty list
   
       for each cell in active list:
           if cell is not movable or is already stable:
               continue
   
           if movement possible (check down, left-down, right-down):
               move object one step to destination
               mark destination as unstable
               add destination to new_active
               // Dependency propagation:
               for each cell in the upper neighborhood (left-up, up, right-up):
                   if contains movable object and is marked stable:
                       mark as unstable
                       add to new_active
           else:
               if no support (i.e., at least one of down, left-down, right-down is empty):
                   // 保持不稳定，重新加入待检查队列
                   add cell to new_active
               else:
                   // 支撑完备：标记为稳定，不再加入
                   mark cell as stable
   
       active list = new_active
   ```
   该方法尽可能仅对受到支撑变化影响的区域进行检查，从而在大多数情况下比全盘扫描更高效。

## 文件结构

- **simulate_drop.c**  
  实现了四种模拟掉落的函数：
  - `simulate_drop_multi`：全盘多次遍历法。
  - `simulate_drop_single`：单次遍历，直接到底法。
  - `simulate_drop_queue`：基于队列驱动的事件驱动法。
  - `simulate_drop_active`：基于稳定标记与依赖传播的局部更新法（如上文代码）。

- **utils.py**  
  包含 Python 辅助函数及工具：
  - `IntArrayType`：将 Python 数组转换为 C 数组。
  - `show_matrix`：将一维数组以矩阵形式显示（旋转90°以使 row=0 为底部）。
  - 封装了上述 C 函数的调用接口（如 `simulate_drop_multi`, `simulate_drop_single`, `simulate_drop_queue`, `simulate_drop_active`）。
  - `run_test_case_all_methods`：自动运行测试用例，调用所有模拟函数并记录运行时间、比较结果。

- **test_dll.ipynb**  
  Jupyter Notebook 测试脚本：
  - 加载动态库。
  - 定义各种测试用例（全空、全障碍、单行、单列、小随机、中随机、大矩阵）。
  - 调用 `run_test_case_all_methods` 运行测试，并打印各算法的运行时间和最终矩阵，比较不同算法之间的结果一致性。

## 结果与讨论

测试结果中显示，对于简单场景（如 AllZero、AllOnes、SingleRow、SingleCol）四种算法均能得到一致的最终矩阵。但在随机测试（RandomMed、RandomBig）中，出现了不同方法最终结果不一致的情况，例如：
- `simulate_drop_multi` 与 `simulate_drop_single` 可能产生不同的结果；
- `simulate_drop_queue` 与 `simulate_drop_active` 也可能与全盘扫描版本不同；
- 一般来说，`simulate_drop_single` 的结果与`simulate_drop_queue`较为一致。

**原因分析**：
1. **更新顺序的差异**  
   - `simulate_drop_multi` 采用全盘多次扫描，严格按照从左下到右上的顺序更新。  
   - `simulate_drop_single` 对每个物体直接跳至最终位置，其处理方式忽略了中间状态。
   - 队列驱动 (`simulate_drop_queue`) 与局部更新 (`simulate_drop_active`) 版本由于只更新活跃区域，其激活顺序和依赖传播机制可能导致处理顺序与全盘扫描不同，从而在存在多个相互依赖物体时产生不同的中间状态和最终结果。

2. **局部更新策略和依赖传播**  
   - 在 `simulate_drop_active` 中，通过稳定标记与依赖传播，仅在下方支撑变化时激活上方邻域。  
   - 如果某些物体已经被确认稳定，则它们不再检查；但如果支撑条件变化传播不完全或处理顺序略有差异，可能会导致部分物体没有被重新检查，最终结果与全盘扫描的版本不同。

个人认为，有些差异（如`multi`与`single`）并非算法实现错误，而是不同模拟策略在处理相邻物体滑动、碰撞时的细微顺序差异导致的。实际应用中，可以根据需要选择更符合物理预期或性能要求的算法版本；
有些差异（如`multi`与`active`）是对于细节思考存在欠缺，存在进一步优化空间。

## 总结

- 本项目实现了四种掉落模拟算法，各自有不同的实现思路和性能特性。
- 通过 Python 的 `ctypes` 和 Jupyter Notebook 进行测试，可以直观地对比各算法的运行时间和结果。
- 关于最终结果的差异，主要是由于不同更新顺序和局部更新策略的差异引起的，若有充足的时间，也许可以进行更细致的优化。

最后，希望这份文档能帮助面试官快速了解项目完成情况、实现思路和细节设计。 



