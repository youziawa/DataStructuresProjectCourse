#ifndef CATALOG_CATALOG_H
#define CATALOG_CATALOG_H

#include "linklist.h"
#include "mystr.h"
#include "seqlist.h"

/*
 * 目录的存储：树的孩子兄弟表示，物理上是二叉链表。
 *
 *   firstChild  = 左孩子 = 第一个孩子（长子）
 *   nextSibling = 右孩子 = 下一个兄弟
 *
 * 例如目录 A 下面依次有 B、C：
 *   A.firstChild = B，B.nextSibling = C，C.nextSibling = NULL。
 * 转成二叉树之后就是 A.lchild = B，B.rchild = C。
 *
 * 多门课程是一个森林。下一棵树挂在当前根的 nextSibling 上，
 * 所以整片森林从第一棵根出发，本身就是一棵二叉树：根的右链就是其余的树。
 *
 * 孩子兄弟链表和二叉树是同一套指针的两种读法。本程序仍复制结点做显式互转，
 * 这样屏幕上可以同时看到森林和二叉树，并能转回去检查是否可逆。
 * 转换时间 O(n)，额外空间 O(n)；直接在孩子兄弟树上遍历则不必复制，额外空间只有遍历栈 O(h)。
 *
 * parent 不改变孩子兄弟定义，只用来在 O(h) 时间内把祖先压栈。
 * 输出路径时从目标压到根，再依次弹出，得到从根到目标的顺序。
 */

typedef enum {
    NK_DIR = 0, // 文件夹
    NK_FILE = 1 // 文件
} NodeKind;

// 目录操作的返回码。菜单和测试都根据它打印原因，不把错误码散落成魔法数。
enum {
    CAT_OK = 0, // 成功
    CAT_NOT_FOUND = -1, // 路径中某一层不存在
    CAT_DUP = -2, // 同一父结点下已经有同名孩子
    CAT_TYPE = -3, // 例如往文件下面再建子结点，或给目录改大小
    CAT_INVALID = -4, // 空路径、连续斜杠、名字太长、大小为负
    CAT_NOMEM = -5 // malloc 失败
};

// 孩子兄弟结点。一棵树或森林中的一个文件夹/文件。
typedef struct CSNode {
    char name[MAX_NAME];
    NodeKind kind;
    long size; // 文件字节数；目录恒为 0
    char ext[MAX_EXT]; // 文件扩展名，不含点；目录为空串
    struct CSNode *parent; // 父结点；森林中各根的 parent 为 NULL
    struct CSNode *firstChild; // 长子，对应二叉树的左孩子
    struct CSNode *nextSibling; // 右兄弟，对应二叉树的右孩子
} CSNode;

// 森林：firstRoot 是第一棵树，其余树通过 nextSibling 串起来。
typedef struct {
    CSNode *firstRoot;
} Forest;

// 显式二叉树结点。lchild 是长子，rchild 是右兄弟，不再使用 parent。
typedef struct BiNode {
    char name[MAX_NAME];
    NodeKind kind;
    long size;
    char ext[MAX_EXT];
    struct BiNode *lchild;
    struct BiNode *rchild;
} BiNode;

// 遍历时对每个结点名调用一次。ctx 由调用方传入，用来收集序列或打印。
typedef void (*WalkFn)(void *ctx, const char *name);

// 置为空森林。
void forest_init(Forest *f);
// 释放森林中的每棵树。之后 firstRoot 为 NULL，可以再次插入。
void forest_destroy(Forest *f);

/*
 * 用串运算把路径拆成各级名字。分隔符是 '/' 或 '\\'。
 * 允许首尾分隔符和名字两侧的空格，例如 " /a/b/ " 得到 a、b。
 * 连续分隔符视为空分量，返回 CAT_INVALID。
 * 成功返回分量个数；非法返回 CAT_INVALID；内存不足返回 CAT_NOMEM。
 */
int split_path(const char *path, char parts[][MAX_NAME], int max_parts);
// 把返回码变成可以打印的中文说明。
const char *catalog_err(int code);

/*
 * 新建一个文件夹。只创建路径的最后一层，父路径必须已经存在。
 * 这样“路径不存在”可以被测试到，而不是悄悄把中间目录建出来。
 * 单层名字会成为森林中的一棵新树。同层重名返回 CAT_DUP。
 */
int catalog_mkdir(Forest *f, const char *path);
// 新建文件。size 必须 >= 0。规则与 mkdir 相同，结点类型是文件。
int catalog_touch(Forest *f, const char *path, long size);
// 删除文件或目录。删除目录时释放整棵子树，不删除它的右兄弟。
int catalog_delete(Forest *f, const char *path);
// 重命名最后一层。新名字不能含分隔符，也不能和同层其它结点重名。文件会重算扩展名。
int catalog_rename(Forest *f, const char *path, const char *new_name);
// 修改文件大小。目标必须是文件。
int catalog_set_size(Forest *f, const char *path, long size);
// 按路径查找。找不到或路径非法时返回 NULL。
CSNode *catalog_find(Forest *f, const char *path);

/*
 * 用顺序栈生成“从根到该结点”的路径，写入 out。
 * 先沿 parent 从目标走到根，全部入栈；再弹出，弹出顺序就是从根到目标。
 * 时间 O(h)，h 为结点深度；栈空间 O(h)。
 */
int catalog_path_string(const CSNode *node, MyString *out);
// 把入栈、出栈过程打印出来，便于答辩时看到栈的作用。
void catalog_show_path(const CSNode *node);
// 打印结点类型、路径、大小或子结点数。
void catalog_show_node(Forest *f, const char *path);
// path 为空时打印整片森林；否则打印该目录的直接孩子。
void catalog_show_dir(Forest *f, const char *path);
// 按缩进打印森林。目录没有孩子时标成空目录。
void catalog_print_forest(const Forest *f);

/*
 * 收集某个目录的直接文件孩子，不含子目录。
 * files、links 都可以传 NULL。两者都收集时，顺序表用尾插、链表也用尾插，次序一致。
 * 目录不存在返回 CAT_INVALID，目标是文件返回 CAT_TYPE。
 */
int catalog_collect_level(const CSNode *dir, SeqList *files, LinkList *links);
/*
 * 在全部文件名上做朴素模式匹配，命中的文件追加到 out。
 * 关键词为空返回 CAT_INVALID。只匹配文件，不匹配文件夹名。
 */
int catalog_collect_keyword(const Forest *f, const char *keyword, SeqList *out);
// 按扩展名精确匹配，区分大小写。ext 为空串时收集没有扩展名的文件。
int catalog_collect_ext(const Forest *f, const char *ext, SeqList *out);
// 打印关键词查找的命中位置和每次匹配的比较次数。
void catalog_show_keyword(const Forest *f, const char *keyword);
// 打印某个扩展名下的全部文件。
void catalog_show_ext(const Forest *f, const char *ext);

// 森林中的结点数，含目录和文件。空森林为 0。
int catalog_node_count(const Forest *f);
// 递归比较名字、类型、大小、扩展名以及孩子兄弟结构。不比较指针地址。
int forest_equal(const Forest *a, const Forest *b);
// 检查每个孩子的 parent 是否指回父结点，根的 parent 是否为 NULL。
int forest_parents_ok(const Forest *f);

/*
 * 下面几组遍历都用顺序栈，时间 O(n)，栈空间最坏 O(n)。
 * cs_preorder：按二叉树先序走孩子兄弟链表（先右兄弟入栈，再长子入栈）。
 *   从第一棵根调用时，序列等于整个森林的先根次序，也等于转换后二叉树的先序。
 * cs_tree_preorder：只遍历以 root 为根的那一棵树，不走到 root 自己的兄弟。
 * cs_tree_postorder：一般树的后序，先访问各子树，最后访问根。
 *   它不等于二叉树后序，因为二叉树的右孩子是兄弟，不是子树。
 */
void cs_preorder(const CSNode *root, WalkFn fn, void *ctx);
void cs_tree_preorder(const CSNode *root, WalkFn fn, void *ctx);
void cs_tree_postorder(const CSNode *root, WalkFn fn, void *ctx);
// 森林先序 = 从 firstRoot 做 cs_preorder。
void forest_preorder(const Forest *f, WalkFn fn, void *ctx);
// 对每棵树分别做一般树后序，树与树之间按根的兄弟次序。
void forest_tree_postorder(const Forest *f, WalkFn fn, void *ctx);

// 复制森林，得到二叉树。空森林返回 NULL。时间、空间都是 O(n)。
BiNode *forest_to_bi(const Forest *f);
/*
 * 把二叉树转回森林并写入 out。调用前 out 不必初始化，函数内部会 forest_init。
 * 左孩子变成长子，右孩子变成兄弟；兄弟的 parent 与当前结点相同。
 */
int bi_to_forest(const BiNode *root, Forest *out);
// 后序释放二叉树。
void bi_destroy(BiNode *root);

// 二叉树非递归先序：访问结点后先压右孩子，再压左孩子，保证左孩子先出栈。
void bi_preorder(const BiNode *root, WalkFn fn, void *ctx);
// 二叉树非递归中序：一路向左入栈，弹出时访问，再转向右孩子。用来核对左右指针。
void bi_inorder(const BiNode *root, WalkFn fn, void *ctx);
// 二叉树非递归后序：记录上一次访问的结点，右子树已经访问过才访问根。
void bi_postorder(const BiNode *root, WalkFn fn, void *ctx);
// 三个递归版本只用来和栈版本对拍，菜单展示的是非递归实现。
void bi_preorder_rec(const BiNode *root, WalkFn fn, void *ctx);
void bi_inorder_rec(const BiNode *root, WalkFn fn, void *ctx);
void bi_postorder_rec(const BiNode *root, WalkFn fn, void *ctx);

// 用 A 及其孩子 B、C 说明左=长子、右=右兄弟，并打印三种遍历。
void catalog_show_mapping_example(void);
// 打印当前森林的先序、中序、后序，并比较直接遍历和先转换再遍历。
void catalog_show_traversals(const Forest *f);
// 打印森林、二叉树括号表示、左右孩子对照表，再转回森林并检查是否一致。
void catalog_show_conversion(const Forest *f);

/*
 * 装入固定示例：两棵课程树、一个空目录、两个同名“习题.pdf”。
 * 会先清空 f 里的旧数据。
 */
int catalog_load_sample(Forest *f);

#endif
