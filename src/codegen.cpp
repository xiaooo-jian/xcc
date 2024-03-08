#include <codegen.h>

// void Codegen::codegen_print(const char * str){

// }

// 偏移量计算怎么感觉可以放到语法分析一次性做完
int Codegen::align(int n, int align) {
  return (n + align - 1) / align * align;
}

void Codegen::iden_offset(){
    int offset = 0;
    for(auto& val : func->sym_table.table){
        offset += 8;
        val.second.offset = -offset;
    }
    func->stack_size = align(offset, 16);
}

static int jump_count() {
  static int i = 1;
  return i++;
}

void Codegen::codegen(string filename)
{
    outFile.open(filename);
    if (!outFile.is_open())
    {
        cout << "Create the assembly file error!" << endl;
        return;
    }

    iden_offset();

    codegen_init();
    codegenStmt(func->stmts);
    assert(depth == 0);
    codegen_end();

    
    outFile.close();
}

void Codegen::codegen_init()
{
    outFile << "\t.globl main\n";
    outFile << "main:\n";
    // stack
    outFile << "\tpush %rbp\n";
    outFile << "\tmov %rsp, %rbp\n";
    outFile << "\tsub $" << func->stack_size << ", %rsp\n";

}

void Codegen::codegen_end()
{
    outFile << ".L.return:\n";
    outFile << "\tmov %rbp, %rsp\n";
    outFile << "\tpop %rbp\n";
    outFile << "\tret\n";
}

void Codegen::load(int value)
{
    outFile << "\tmov rax, " << value << endl;
}

void Codegen::add(int r1)
{
    outFile << "\tadd rax, " << r1 << endl;
}


void Codegen::push()
{
    outFile << "\tpush %rax" << endl;
    depth++;
}

void Codegen::pop(string arg)
{
    outFile << "\tpop\t" << arg << endl;
    depth--;
}

void Codegen::codegenStmt(vector<AST_node*> stmts){
    for(auto root: stmts){

        switch(root->type){
            case AST_Expr:
                codegenExpr(root->left);
                break;
            case AST_Return:
                root->type = AST_Expr;
                codegenExpr(root->left);
                outFile << "\tjmp .L.return\n";
                break ;
            case AST_Block:
                if(root->childs.size() == 0) 
                    break;
                codegenStmt(root->childs);
                break;
            case AST_If:{

                int c = jump_count();
                codegenExpr(root->cond->left);            
                outFile << "\tcmp $0, %rax\n";
                outFile << "\tje .L.if.else." << c << endl;
                codegenStmt(root->then);
                outFile << "\tjmp .L.if.end." << c << endl;
                outFile << "\n.L.if.else." << c << ":\n";
                if(!root->els.empty()){
                    codegenStmt(root->els);
                }
                outFile << "\n.L.if.end." << c << ":\n";
                break;
            }
            case AST_For:{
                int c = jump_count();
                if(root->init)
                    codegenExpr(root->init);
                outFile << ".L.begin." << c << ":\n";
                if(root->cond){
                    codegenExpr(root->cond->left);
                    outFile << "\tcmp $0, %rax\n";
                    outFile << "\tje .L.end." << c << endl;
                }
                codegenStmt(root->then);
                if(root->expr){
                    codegenExpr(root->expr);
                }
                outFile << "\tjmp .L.begin." << c << endl;
                outFile << "\n.L.end." << c << ":\n";
                break;
            }

            default:
                break;
        }

    }

}


void Codegen::codegenExpr(AST_node *node)
{
    // if(node == NULL) return;
            // cout <<node->type<< endl;
    switch (node->type)
    {
    case AST_Num:
        outFile << "\tmov $" << node->val << ", %rax\n";
        return;
    case AST_val:
        outFile << "\tlea " << func->get_offset(node->name) << "(%rbp), %rax\n";
        outFile << "\tmov (%rax), %rax\n";
        return;
    case AST_Assign:
        outFile << "\tlea " << func->get_offset(node->name) << "(%rbp), %rax\n";
        push();
        codegenExpr(node->left);
        pop("%rdi");
        outFile << "\tmov %rax, (%rdi)\n";
        return;
    default:
        LOG("nothing...");
    }
    
    if(node->right){
        codegenExpr(node->right);
        if(node->type != AST_None )
            push();
    }
    if(node->left){
        codegenExpr(node->left);
        if(node->type != AST_None )
            pop("%rdi");
    }





    switch (node->type)
    {
    case AST_Add:
        outFile << "\tadd %rdi, %rax\n";
        break;
    case AST_Mins:
        outFile << "\tsub %rdi, %rax\n";
        break;
    case AST_Multiply:
        outFile << "\timul %rdi, %rax\n";
        break;
    case AST_Divide:
        outFile << "\tcqo\n"
                << "\tidiv %rdi\n";
        break;
    case AST_Eq:
        outFile << "\tcmp %rdi, %rax\n";
        outFile << "\tsete %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_Neq:
        outFile << "\tcmp %rdi, %rax\n";
        outFile << "\tsetne %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_leq:
        outFile << "\tcmp %rdi, %rax\n";
        outFile << "\tsetle %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_req:
        outFile << "\tcmp %rax,%rdi\n";
        outFile << "\tsetle %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_lt:
        outFile << "\tcmp %rdi, %rax\n";
        outFile << "\tsetl %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_rt:
        outFile << "\tcmp %rax,%rdi\n";
        outFile << "\tsetl %al\n";
        outFile << "\tmovzb %al, %rax\n";
        break;
    case AST_None:
    case AST_Expr:
        break;
    default:
        ERROR("except %d\n",node->type);
        // cout << "error in codegenexpr\n";
    }
}

