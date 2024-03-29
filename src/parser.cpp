#include <parser.h>
#include <symtable.h>

// token匹配
bool Parser::match(TokenType type)
{
    if (tokens[cur].type == type)
    {
        return true;
    }
    return false;
}

// token匹配(不符合报错)
void Parser::skip(TokenType type)
{
    if (tokens[cur].type == type)
    {
        cur++;
        return;
    }
    cout << tokens[cur].col << endl;
    // for(int i = -3 ; i < 3 ;i++){
    //     cout << tokens[cur+i].value << " ";
    // }

    ERROR("Expect %d but get %d", type, tokens[cur].type);
}

// 当前符号表
SymTable *cur_table;

// 新建变量
void Parser::new_ident(AST_node *node)
{
    AST_node *id = new AST_node;
    id->name = tokens[cur].value;
    id->type = AST_val;

    if (cur_table->get(id->name, id->val))
    {
        ERROR("variable [%s] redefined \n", id->name.c_str());
    }
    cur_table->add(Type(TY_int), tokens[cur].value);

    node->left = id;
}
// 语法分析递归下降入口
// Program     =>  Function *
vector<Function *> Parser::parse()
{
    cur = 0;
    vector<Function *> funcs;
    while (!match(Tok_eof))
    {
        funcs.push_back(parserFunction());
    }

    return funcs;
}

// Function    =>  'int' funcname '(' param ')' Stmt *
Function *Parser::parserFunction()
{
    Function *func = new Function();
    cur_table = (SymTable *)&(func->sym_table);
    //'int'
    if (match(Tok_int))
    {
        cur++;
        func->return_type = Type(TY_int);
    }
    // funcname
    if (match(Tok_ident))
    {
        func->name = tokens[cur].value;
        cur++;
    }
    // '(' param ')'
    skip(Tok_lbak);
    while (!match(Tok_rbak))
    { // without parameter
        cur++;
        Type *t = new Type(TY_int);
        while (match(Tok_mul))
        {
            t = new Type(TY_point, t);
            cur++;
        }
        if (match(Tok_ident))
        {
            AST_node *temp = new AST_node();
            temp->op_type = t;
            new_ident(temp);
            cur++;

            func->params.push_back(temp);
            if (match(Tok_comma))
                cur++;
        }
    }
    skip(Tok_rbak);
    // Stmt *
    skip(Tok_lcul);
    while (!match(Tok_rcul))
    {
        func->stmts = parserStmt();
        typeAdd(func->stmts);
    }
    skip(Tok_rcul);
    return func;
}

// Stmt   =>  ExprStmt ';'
//        |   ReturnStmt ';'
//        |    '{'  Stmt * '}'
//        |   ';'
//        |   DeclarStmt
//        |   'for' '(' ExprStmt ';'ExprStmt ';' Stmt ')' stmts
//        |   'if' '(' Expression ')' Stmt ('else' Stmt)?
//        |   'while' '(' Expression ')' Stmt

vector<AST_node *> Parser::parserStmt(int times)
{

    vector<AST_node *> roots;
    while (!match(Tok_rcul) && !match(Tok_eof))
    {
        bool declare_flag = true;
        AST_node *node = new AST_node(); // 要记得用new开辟空间啊啊啊啊
        //'{'  Stmt * '}'
        if (match(Tok_lcul))
        {

            cur++;
            node->childs = parserStmt();
            skip(Tok_rcul);
            node->type = AST_Block;
        }
        // ReturnStmt ';'
        else if (match(Tok_return))
        {
            cur++;
            node = parserExprStmt();
            node->type = AST_Return;
            skip(Tok_seg);
        }
        // ';'
        else if (match(Tok_seg))
        {
            cur++;
        }
        //'if' '(' Expression ')' Stmt ('else' Stmt)?
        else if (match(Tok_if))
        {
            cur++;
            skip(Tok_lbak);
            node->cond = parserExpression();
            skip(Tok_rbak);
            node->then = parserStmt(1);

            // cout << node->then.size() << endl;
            if (match(Tok_else))
            {
                cur++;
                node->els = parserStmt(1); // 之后后面紧跟的域才属于else的部分
            }
            node->type = AST_If;
        }
        //'for' '(' ExprStmt ';'ExprStmt ';' Stmt ')' stmts
        else if (match(Tok_for))
        {

            cur++;
            skip(Tok_lbak);

            if (!match(Tok_seg))
                node->init = parserStmt(1);
            if (match(Tok_seg))
                skip(Tok_seg);
            if (!match(Tok_seg))
                node->cond = parserExprStmt();
            skip(Tok_seg);
            if (!match(Tok_rbak))
                node->expr = parserExprStmt();
            skip(Tok_rbak);
            node->then = parserStmt(1);
            node->type = AST_For;
        }
        //'while' '(' Expression ')' Stmt
        else if (match(Tok_while))
        {
            cur++;
            skip(Tok_lbak);
            node->cond = parserExpression();
            skip(Tok_rbak);
            node->then = parserStmt(1);
            node->type = AST_For;
        }
        // DeclarStmt ';'
        else if (match(Tok_int))
        {
            cur++;
            Type *t = new Type(TY_int);
            while (match(Tok_mul))
            {
                t = new Type(TY_point, t);
                cur++;
            }

            while (match(Tok_ident))
            {

                AST_node *temp = new AST_node();
                temp->op_type = t;

                new_ident(temp);

                cur++;
                if (match(Tok_assign))
                {
                    cur++;
                    temp->type = AST_Assign;
                    temp->right = parserExpression();
                }
                roots.push_back(temp);
                if (match(Tok_comma))
                    cur++;
            }
            skip(Tok_seg);
            declare_flag = false;
        }
        // ExprStmt ';'
        else
        {
            node = parserExprStmt();
            node->type = AST_Expr;
            skip(Tok_seg);
        }
        if (node && declare_flag)
            roots.push_back(node);
        if (times) // 如果是if语句过来的只需要取后面一个语句即可，block看做一个语句
            break;
    }
    return roots;
}

// ExprStmt    =>  Expression
AST_node *Parser::parserExprStmt()
{
    LOG("ExprStmt\n");
    AST_node *node = new AST_node;
    node->left = parserExpression();
    return node;
}

// Expression  =>  EqualExpr ('=' Expression )?
AST_node *Parser::parserExpression()
{
    LOG("Expression\n");
    AST_node *node = new AST_node;
    node->left = parserEqualExpr();
    if (match(Tok_assign))
    {
        cur++;
        node->right = parserExpression();
        node->type = AST_Assign;
    }
    return node;
};

// EqualExpr   =>  CompareExpr (== !=) EqualExpr
AST_node *Parser::parserEqualExpr()
{
    LOG("EqualExpr\n");
    AST_node *node = new AST_node;
    node->left = parserCompareExpr();
    if (match(Tok_eq) || match(Tok_neq))
    {
        node->type = match(Tok_eq) ? AST_Eq : AST_Neq;
        cur++;
        node->right = parserEqualExpr();
    }
    return node;
}

// CompareExpr =>  AddMinsExpr (> < >= <=) CompareExpr
AST_node *Parser::parserCompareExpr()
{
    LOG("CompareExpr\n");
    AST_node *node = new AST_node;
    node->left = parserAddMinsExpr();
    if (match(Tok_leq) || match(Tok_req) || match(Tok_lt) || match(Tok_rt))
    {
        node->type = match(Tok_leq) ? AST_leq : match(Tok_req) ? AST_req
                                            : match(Tok_lt)    ? AST_lt
                                                               : AST_rt;
        cur++;
        node->right = parserCompareExpr();
    }
    return node;
}

// AddMinsExpr =>  MulDivExpr (+ -) AddMinsExpr
AST_type last_op = AST_Add;
AST_node *Parser::parserAddMinsExpr()
{
    LOG("AddMinsExpr\n");
    AST_node *node = new AST_node;
    node->left = parserMulDivExpr();
    if (match(Tok_plus))
    {
        node->type = AST_Add;
        cur++;
        // last_op = AST_Add;
        node->right = parserAddMinsExpr();

        typeAdd(node);

        if (node->left->op_type->ty == TY_int && node->right->op_type->ty == TY_int)
        {
            return node;
        }

        if (!node->left->op_type->base && node->right->op_type->base)
        {
            AST_node *temp = node->left;
            node->left = node->right;
            node->right = temp;
        }

        AST_node *new_right = new AST_node();
        new_right->type = AST_Multiply;

        AST_node *num = new AST_node();
        num->type = AST_Num;
        num->val = 8;

        new_right->left = num;
        new_right->right = node->right;
        node->right = new_right;
    }
    else if (match(Tok_minus))
    {
        node->type = AST_Mins;
        cur++;
        // last_op = AST_Mins;
        node->right = parserAddMinsExpr();
        typeAdd(node);
        // num-num
        if (node->left->op_type->ty == TY_int && node->right->op_type->ty == TY_int)
        {
            return node;
        }
        // ptr - num
        if (node->left->op_type->base && node->right->op_type->ty == TY_int)
        {
            AST_node *new_right = new AST_node();
            new_right->type = AST_Multiply;

            AST_node *num = new AST_node();
            num->type = AST_Num;
            num->val = 8;

            new_right->left = num;
            new_right->right = node->right;
            node->right = new_right;
            typeAdd(node);
            return node;
        }
        // ptr-ptr
        if (node->left->op_type->base && node->right->op_type->base)
        {
            AST_node *new_right = new AST_node();
            new_right->type = AST_Divide;

            AST_node *num = new AST_node();
            num->type = AST_Num;
            num->val = 8;

            new_right->left = node;
            new_right->right = num;
            node = new_right;
            return node;
        }
    }
    return node;
}

// MulDivExpr  =>  Unary  (* /) MulDivExpr
AST_node *Parser::parserMulDivExpr()
{
    LOG("MulDivExpr\n");
    AST_node *node = new AST_node;
    node->left = parserUnaryExpr();
    if (match(Tok_mul) || match(Tok_div))
    {
        // if(last_op == AST_Divide )
        //     node->type = match(Tok_mul) ? AST_Divide : AST_Divide;
        // else
        //     node->type = match(Tok_mul) ? AST_Multiply : AST_Divide;
        node->type = match(Tok_mul) ? AST_Multiply : AST_Divide;
        cur++;
        last_op = AST_Divide;
        node->right = parserMulDivExpr();
    }
    return node;
}

AST_node *Parser::parserUnaryExpr()
{
    LOG("Unary\n");

    int op = 1;
    AST_node *node = new AST_node();
    if (match(Tok_minus) || match(Tok_plus))
    {
        if (match(Tok_minus))
        {
            op *= -1;
            cur++;
        }
        else if (match(Tok_plus))
        {
            cur++;
        }
        node = parserUnaryExpr();
        node->val = op * node->val;
    }
    else if (match(Tok_mul))
    {
        cur++;
        node->left = parserUnaryExpr();
        node->type = AST_Ref;
    }
    else if (match(Tok_addr))
    {
        cur++;
        node->left = parserUnaryExpr();
        node->type = AST_Addr;
    }
    else
    {
        node = parserPrimary();
    }

    return node;
}

// Unary       =>  (- + * &) Unary | Primary
AST_node *Parser::parserPrimary()
{
    LOG("primary\n");
    // cout << tokens[cur].value << endl;
    AST_node *node = new AST_node;

    if (match(Tok_num))
    {
        node->type = AST_Num;
        node->val = atoi(tokens[cur].value.c_str());
        cur++;
    }
    else if (match(Tok_lbak))
    {
        cur++;
        node = parserExpression();
        if (match(Tok_rbak))
        {
            cur++;
        }
    }
    else if (match(Tok_ident))
    {
        if (tokens[cur + 1].type != Tok_lbak)
        {
            node->name = tokens[cur].value;
            node->type = AST_val;
            if (!cur_table->get(node->name, node->val))
            {
                ERROR("variable [%s] not defined \n", node->name.c_str());
            }
            cur++;
        }
        else
        {
            node->name = tokens[cur].value;
            node->type = AST_Func;
            cur++;
            skip(Tok_lbak);
            while (!match(Tok_rbak))
            {
                node->init.push_back(parserExpression());
                if (match(Tok_comma))
                {
                    cur++;
                }
            }
            skip(Tok_rbak);
        }
    }
    else
    {
        cout << tokens[cur].line << " " << tokens[cur].col << endl;
        ERROR("parser error for get [%s] \n", tokens[cur].value.c_str());
    }
    return node;
}

// Primary     =>  Num | '(' Expression ')' | ident
void Parser::parserDisplay(AST_node *node, int n = 0)
{
    if (!node)
        return;

    // if(node->right){
    parserDisplay(node->right, n + 1);
    // }
    // if (node->left)
    // {
    parserDisplay(node->left, n + 1);
    // }

    cout << string(n, ' ') << node->val << " type:" << node->type << endl;
}