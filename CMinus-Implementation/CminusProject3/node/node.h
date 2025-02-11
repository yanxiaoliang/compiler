#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <typeinfo>
using namespace std;


class CodeGenContext {
public:
  vector<string> constantString;
  vector<string> regList;
  map<string,int> usedReg;
  map<string,string>aliasname;
  int numofvariable;
  int currentOffset;
  map<string,int>varoffset;
  int readforvariable;
  CodeGenContext() {prevlReg=0,prevrReg=0,readforvariable=0
      ,startSeg=false,segIndex=0,currentOffset=0;}
  int prevlReg;
  int prevrReg;
  bool startSeg;
  int segIndex;
  int getReg(int i=2){
    for(;i<regList.size();i++){
      if(usedReg[regList[i]]==0){
	usedReg[regList[i]]=1;
	return i;
      }		
    }	
  }
  void setReg(int i=2){
    for(;i<regList.size();i++)
  	usedReg[regList[i]]=0;

    usedReg["ebx"]=0;
    usedReg["ecx"]=0;
    usedReg["edx"]=0;

  }

  void setRegZero(int i){
    usedReg[regList[i]] = 0;
  }
  void setRegOne(int i){
    usedReg[regList[i]] = 1;
  }
};

class NStatement;
class NExpression;

class NVariableDeclaration;
class NBlock;

typedef vector<NStatement*> StatementList;
typedef vector<NExpression*> ExpressionList;
typedef vector<NVariableDeclaration*> VariableList;
typedef vector<NBlock*> BlockList;


class Node {
 public:
  virtual ~Node() {}
  virtual void codeGen(CodeGenContext& context) { }
};

class NExpression : public Node {
 public:
  int reg;		
  virtual void codeGen(CodeGenContext& context) { }
};

class NStatement : public Node {
 public:
  bool startSeg;
  int segIndex;
  NStatement(){startSeg = false,segIndex=0;}
  virtual void codeGen(CodeGenContext& context) { }
};

class NInteger : public NExpression {
 public:
  long long value;
 NInteger(long long value) : value(value) { }
  virtual void codeGen(CodeGenContext& context){
    cout<<"	movl $"<<value<<", \%"<<context.regList[reg]<<endl;
    context.startSeg = false;
  }		
};


class NDouble : public NExpression {
 public:
  double value;
 NDouble(double value) : value(value) { }
  virtual void codeGen(CodeGenContext& context){
  }
};

class NString : public NExpression {
 public:
  string name;
 NString(const string& name) : name(name) { }
  virtual void codeGen(CodeGenContext& context){
    
    cout<<"	movl $"<<context.aliasname[name]<<", \%ebx"<<endl;
    context.startSeg = false;
    cout<<"	movl %ebx, \%esi"<<endl;
    cout<<"	movl $0, \%eax"<<endl;
    cout<<"	movl $.str_wformat, %edi"<<endl;
    cout<<"	call printf"<<endl;
  }
};

class NIdentifier : public NExpression {
 public:
  string name;
  int type;
  int capacity;
  bool isArray;
  NExpression* offset;
 NIdentifier(const string& name,int type=0) : name(name),type(type){ 
    capacity = 1;
    isArray = false;
 }
  virtual void codeGen(CodeGenContext& context){
    if(isArray){
      string tmp = context.usedReg["rbx"]==0 &&
	context.usedReg["ebx"]==0?"rbx":(context.usedReg["rcx"]==0 &&
	context.usedReg["ecx"]==0?"rcx":"rdx");   

      cout<<"	movq $_gp,\%"<<tmp<<endl;
      context.startSeg = false;
      cout<<"	addq $"<<context.varoffset[name]<<", \%"<<tmp<<endl;

      context.usedReg[tmp]=1;

      offset->reg = context.getReg(5);
      context.prevrReg = offset->reg;

      int tmpRead = context.readforvariable;
      context.readforvariable = 0;

      offset->codeGen(context);

      context.readforvariable= tmpRead;


      cout<<"	imull $4, %"<<context.regList[offset->reg]<<endl;
      cout<<"	movslq %"<<context.regList[offset->reg]<<", %"<<context.regList[offset->reg].substr(0,context.regList[offset->reg].size()-1) <<endl;
      cout<<"	addq %"<<context.regList[offset->reg].substr(0,context.regList[offset->reg].size()-1) <<", \%"<<tmp<<endl;

      context.usedReg[tmp]=0;
      context.setRegZero(offset->reg);

      if(context.readforvariable)
	return;
      cout<<"	movl (%"<<tmp<<"), %"<<context.regList[reg]<<endl;		
    }else{
      string tmp = context.usedReg["rbx"]==0 &&
	context.usedReg["ebx"]==0?"rbx":(context.usedReg["rcx"]==0 &&
	context.usedReg["ecx"]==0?"rcx":"rdx");   

      cout<<"	movq $_gp,\%"<<tmp<<endl;
      context.startSeg = false;
      cout<<"	addq $"<<context.varoffset[name]<<", \%"<<tmp<<endl;
      if(context.readforvariable)
	return;
      cout<<"	movl (%"<<tmp<<"), %"<<context.regList[reg]<<endl;		
    }
  }
};

class NMethodCall : public NExpression {
 public:
  const NIdentifier& id;
  ExpressionList arguments;
 NMethodCall(const NIdentifier& id, ExpressionList& arguments) :
  id(id), arguments(arguments) { }
 NMethodCall(const NIdentifier& id) : id(id) { }
  virtual void codeGen(CodeGenContext& context){
  }
};

class NBinaryOperator : public NExpression {
 public:
  string op;
  NExpression& lhs;
  NExpression& rhs;
 NBinaryOperator(NExpression& lhs, string op, NExpression& rhs) :
  lhs(lhs), rhs(rhs), op(op) { }
  virtual void codeGen(CodeGenContext& context){
    if(op=="+" || op=="-" || op=="*"){
      lhs.reg = context.prevrReg?context.prevrReg
	:(context.prevlReg?context.prevlReg:context.getReg(5)); 
      if(typeid(lhs)==typeid(NBinaryOperator))
	context.prevlReg = lhs.reg;
      lhs.codeGen(context);
      if(context.prevrReg)
	context.prevrReg = 0;     
      else
	context.prevlReg = 0;     

      rhs.reg = context.getReg(5);
      if(typeid(rhs)==typeid(NBinaryOperator))
	context.prevrReg = rhs.reg;
      rhs.codeGen(context);

      if(op=="+")
	cout<<"	addl \%"<<context.regList[rhs.reg]<<", \%"<<context.regList[lhs.reg]<<endl;
      else if(op=="-")
      cout<<"	subl \%"<<context.regList[rhs.reg]<<", \%"<<context.regList[lhs.reg]<<endl;
      else if(op=="*")
      cout<<"	imull \%"<<context.regList[rhs.reg]<<", \%"<<context.regList[lhs.reg]<<endl;
	
      context.setRegZero(rhs.reg);
    }

    if(op=="/"){

      lhs.reg = context.prevrReg?context.prevrReg
	:(context.prevlReg?context.prevlReg:context.getReg(5)); 
      if(typeid(lhs)==typeid(NBinaryOperator))
	context.prevlReg = lhs.reg;
      lhs.codeGen(context);
      if(context.prevrReg)
	context.prevrReg = 0;     
      else
	context.prevlReg = 0;     

      rhs.reg = context.getReg(5);
      if(typeid(rhs)==typeid(NBinaryOperator))
	context.prevrReg = rhs.reg;
      rhs.codeGen(context);

      string tmp = context.usedReg["ebx"]==0 && 
	context.usedReg["rbx"]==0?"ebx":"ecx"; 

      cout<<"	movl \%"<<context.regList[lhs.reg]<<", %eax"<<endl;
      cout<<"	movl \%"<<context.regList[rhs.reg]<<", \%"<<tmp<<endl;
      cout<<"	cdq"<<endl;
      cout<<"	idivl \%"<<tmp<<endl;
      cout<<"	movl \%eax"<<", \%"<<context.regList[lhs.reg]<<endl;
      context.setRegZero(rhs.reg);
    }


    if(op=="&&" || op=="||" || op=="^"){

      lhs.reg = context.prevrReg?context.prevrReg
	:(context.prevlReg?context.prevlReg:context.getReg(5)); 
      if(typeid(lhs)==typeid(NBinaryOperator))
	context.prevlReg = lhs.reg;
      lhs.codeGen(context);
      if(context.prevrReg)
	context.prevrReg = 0;     
      else
	context.prevlReg = 0;     

      rhs.reg = context.getReg(5);
      if(typeid(rhs)==typeid(NBinaryOperator))
	context.prevrReg = rhs.reg;
      rhs.codeGen(context);

      string tmp = context.usedReg["ebx"]==0 && 
	context.usedReg["rbx"]==0?"ebx":"ecx"; 

      cout<<"	movl \%"<<context.regList[lhs.reg]<<", %eax"<<endl;
      cout<<"	movl \%"<<context.regList[rhs.reg]<<", \%"<<tmp<<endl;
      if(op=="&&")
	cout<<"	andl  \%eax, \%"<<tmp<<endl;
      else if(op=="||")
	cout<<"	orl  \%eax, \%"<<tmp<<endl;
      else if(op=="^")
	cout<<"	xorl  \%eax, \%"<<tmp<<endl;

      cout<<"	movl \%"<<tmp<<", \%"<<context.regList[lhs.reg]<<endl;
      context.setRegZero(rhs.reg);
    }


    if(op=="EQ" || op=="GT" || op=="GE"|| op=="LT" 
       || op=="LE"|| op=="NE" || op=="!"){

      lhs.reg = context.prevrReg?context.prevrReg
	:(context.prevlReg?context.prevlReg:context.getReg(5)); 
      if(typeid(lhs)==typeid(NBinaryOperator))
	context.prevlReg = lhs.reg;
      lhs.codeGen(context);
      if(context.prevrReg)
	context.prevrReg = 0;     
      else
	context.prevlReg = 0;     

      rhs.reg = context.getReg(5);
      if(typeid(rhs)==typeid(NBinaryOperator))
	context.prevrReg = rhs.reg;
      rhs.codeGen(context);

      string tmp = context.usedReg["ebx"]==0 && 
	context.usedReg["rbx"]==0?"ebx":"ecx"; 

      cout<<"	movl \%"<<context.regList[lhs.reg]<<", %eax"<<endl;
      cout<<"	movl \%"<<context.regList[rhs.reg]<<", \%"<<tmp<<endl;

      cout<<"	cmpl  \%"<<tmp<<", \%eax"<<endl;

      cout<<"	movl $0"<<", \%"<<tmp<<endl;
      cout<<"	movl $1"<<", \%eax"<<endl;

      if(op=="!"){
	cout<<"	cmove \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="EQ"){
	cout<<"	cmove \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="LT"){
	cout<<"	cmovl \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="LE"){
	cout<<"	cmovle \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="GE"){
	cout<<"	cmovge \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="GT"){
	cout<<"	cmovg \%eax"<<", \%"<<tmp<<endl;
      }else if(op=="NE"){
	cout<<"	cmovne \%eax"<<", \%"<<tmp<<endl;
      }

      cout<<"	movl \%"<<tmp<<", \%"<<context.regList[lhs.reg]<<endl;
      context.setRegZero(rhs.reg);
    }        
  }

};

class NIOStatement : public NExpression {
 public:	
  NIdentifier& id;
  NExpression& arguments;	
 NIOStatement(NIdentifier& id, NExpression& arguments) :
  id(id), arguments(arguments) {}
  virtual void codeGen(CodeGenContext& context){
    if(id.name=="write"){		
      if(typeid(arguments)==typeid(NBinaryOperator)){
	arguments.reg=context.getReg();		
	context.prevlReg = arguments.reg;
      }
      else{
	arguments.reg=2;
	context.setRegOne(2);
      }
      arguments.codeGen(context);
      if(id.type==1){	
	cout<<"	movl %"<<context.regList[arguments.reg]<<", \%esi"<<endl;
	cout<<"	movl $0, \%eax"<<endl;
	cout<<"	movl $.int_wformat, %edi"<<endl;
	cout<<"	call printf"<<endl;		
      }
      context.setRegZero(arguments.reg);
    }else{
      context.readforvariable=1;
      arguments.codeGen(context);
      context.readforvariable=0;
      cout<<"	movl $.int_rformat, \%edi"<<endl;
      cout<<"	movl \%ebx, \%esi"<<endl;
      cout<<"	movl $0, \%eax"<<endl;
      cout<<"	call scanf"<<endl;
    }
    context.setReg();
  }
}; 


class NAssignment : public NExpression {
 public:
  NIdentifier& lhs;
  NExpression& rhs;
 NAssignment(NIdentifier& lhs, NExpression& rhs) :
  lhs(lhs), rhs(rhs) { }
  virtual void codeGen(CodeGenContext& context){

    rhs.reg = context.getReg(5);
    context.prevlReg = rhs.reg;

    context.readforvariable=1;		
    lhs.codeGen(context);

    context.readforvariable=0;		
    context.usedReg["rbx"]=1;
    context.prevrReg = rhs.reg;
    rhs.codeGen(context);
    cout<<"	movl \%"<<context.regList[rhs.reg]<<", (\%rbx)"<<endl;

    context.usedReg["rbx"]=0;
    context.setRegZero(rhs.reg);	
    context.setReg();
  }
};


class NWHILEStatement : public NStatement {
 public:
  NExpression& expression;
  StatementList whileStates;

 NWHILEStatement(NExpression& expression) :
  expression(expression) { }

void codeGen(CodeGenContext& context){

    expression.reg=context.getReg(5);		
    context.prevlReg = expression.reg;
    context.segIndex++;
    int headSeg = context.segIndex;
    cout<<".L"<<headSeg<<":"<<endl;
    expression.codeGen(context);
    cout<<"	movl $0"<<", \%eax"<<endl;
    cout<<"	cmpl \%eax"<<", \%"<<context.regList[expression.reg]<<endl;

    cout<<"	je .L"<<this->segIndex<<endl;

    context.setRegZero(expression.reg);	

    StatementList::const_iterator it;

    for (it = whileStates.begin(); it != whileStates.end(); it++) {

      string tmp;
      tmp+= typeid(**it).name();

      if(typeid(**it)==typeid(NWHILEStatement) 
	 || tmp == "12NIFStatement"){
	context.segIndex++;
	if(!(**it).startSeg)
	  (**it).segIndex = context.segIndex;
	(**(it+1)).segIndex = context.segIndex;
	(**(it+1)).startSeg = true;
      }



      if((**it).startSeg)
	cout<<".L"<<(**it).segIndex<<":"<<endl;


      if((**it).startSeg && tmp == "12NIFStatement")
	  (**it).segIndex=context.segIndex;

      (**it).codeGen(context);
      context.setReg();
    }

    cout<<"	jmp .L"<<headSeg<<endl;
  }
};

class NIFStatement : public NStatement {
 public:
  NExpression& expression;
  StatementList ifStates;
  StatementList elseStates;

 NIFStatement(NExpression& expression) :
  expression(expression) { }
void codeGen(CodeGenContext& context){
    expression.reg=context.getReg(5);		
    context.prevlReg = expression.reg;
    expression.codeGen(context);

    cout<<"	movl $0"<<", \%eax"<<endl;
    cout<<"	cmpl \%eax"<<", \%"<<context.regList[expression.reg]<<endl;

    if(elseStates.size()>0){
      context.segIndex++;
      this->segIndex=context.segIndex;
    }

    cout<<"	je .L"<<this->segIndex<<endl;
    context.setRegZero(expression.reg);	

    bool lastJump = false;
    int tailSeg = 0;

    for(int i=0;i<ifStates.size();i++){

      if(typeid(*ifStates[i])==typeid(NWHILEStatement)||
	   typeid(*ifStates[i])==typeid(NIFStatement)){
	context.segIndex++;
	if(i<ifStates.size()-1){
	  ifStates[i+1]->startSeg = true;
	  ifStates[i+1]->segIndex = context.segIndex;
	}else{
	  lastJump = true;
	  tailSeg = context.segIndex;
	}

	if(!ifStates[i]->startSeg)
	  ifStates[i]->segIndex = context.segIndex;
      }

      if(ifStates[i]->startSeg){
	cout<<".L"<<ifStates[i]->segIndex<<":"<<endl;
	context.startSeg = true;
      }

      ifStates[i]->codeGen(context);
      context.startSeg = false;
    }


    if(elseStates.size()>0){
      if(lastJump)
	cout<<".L"<<tailSeg<<":"<<endl;

      cout<<"	jmp .L"<<this->segIndex-1<<endl;
      StatementList::const_iterator it;
      cout<<".L"<<this->segIndex<<":"<<endl;


      lastJump = false;
      tailSeg = 0;

      for(int i=0;i<elseStates.size();i++){
	if(typeid(*elseStates[i])==typeid(NWHILEStatement)||
	   typeid(*elseStates[i])==typeid(NIFStatement)){
	  context.segIndex++;
	  if(i<elseStates.size()-1){
	    elseStates[i+1]->startSeg = true;
	    elseStates[i+1]->segIndex = context.segIndex;
	  }else{
	    lastJump = true;
	    tailSeg = context.segIndex;
	  }

	  if(!elseStates[i]->startSeg)
	    elseStates[i]->segIndex = context.segIndex;
	}

	if(elseStates[i]->startSeg){
	  cout<<".L"<<elseStates[i]->segIndex<<":"<<endl;
	  context.startSeg = true;
	}

	elseStates[i]->codeGen(context);
	context.startSeg = false;
      }

      if(lastJump){
       cout<<".L"<<tailSeg<<":"<<endl;
      }     
    }
  }

};


class NBlock : public NExpression {
 public:
  StatementList statements;
  VariableList  localVariables;
  NBlock() { }
  virtual void codeGen(CodeGenContext& context){
    bool lastJump = false;
    int tailSeg = 0;

    for(int i=0;i<statements.size();i++){
      if(typeid(*statements[i])==typeid(NIFStatement)||
	 typeid(*statements[i])==typeid(NWHILEStatement)){
	context.segIndex++;
	if(i<statements.size()-1){
	  statements[i+1]->startSeg = true;
	  statements[i+1]->segIndex = context.segIndex;
	}else{
	  lastJump = true;
	  tailSeg = context.segIndex;
	}
	if(!statements[i]->startSeg)
	  statements[i]->segIndex = context.segIndex;
      }

      if(statements[i]->startSeg){
	cout<<".L"<<statements[i]->segIndex<<":"<<endl;
	context.startSeg = true;
      }

      statements[i]->codeGen(context);
      context.startSeg = false;
      context.setReg();
    }

    if(lastJump){
      cout<<".L"<<tailSeg <<":	nop"<<endl;
    }

  }
};


class NVariableDeclaration : public NStatement {
 public:
  NIdentifier& id;
  //const NIdentifier& type;
  NExpression *assignmentExpr;
 NVariableDeclaration(NIdentifier& id) : id(id) {}
  //NVariableDeclaration(const NIdentifier& type, NIdentifier& id) :
  //    type(type), id(id) { }
  //NVariableDeclaration(const NIdentifier& type, NIdentifier& id, NExpression *assignmentExpr) :
  //    type(type), id(id), assignmentExpr(assignmentExpr) { }
  virtual void codeGen(CodeGenContext& context){}
};

class NProgram : public NExpression {
 public:
  BlockList blocks;
  VariableList  variables;
  NProgram() { }
  virtual void codeGen(CodeGenContext& context){
    cout<<"	.section	.rodata"<<endl;
    cout<<"	.int_wformat: .string \"\%d\\n\""<<endl;
    cout<<"	.str_wformat: .string \"%s\\n\""<<endl;
    for(int i=0;i<context.constantString.size();i++){
      string tmp(".string_const");
      tmp+=i+'0';
      context.aliasname[context.constantString[i]] = tmp;		
      cout<<"	.string_const"<<i<<": .string \""<<context.constantString[i]<<"\""<<endl;
    }
    cout<<"	.int_rformat: .string \"%d\""<<endl;


    context.numofvariable = 0 ;
    for(int i=0;i<variables.size();i++){
      if(variables[i]->id.isArray)
	context.numofvariable+=variables[i]->id.capacity;
      else
	context.numofvariable++;
    }

    if(context.numofvariable)	
      cout<<"	.comm _gp, "<<context.numofvariable*sizeof(int)<<", "<<sizeof(int)<<endl;

    cout<<"	.text"<<endl;
    cout<<"	.globl main"<<endl;
    cout<<"	.type main,@function"<<endl;
    cout<<"main:	nop"<<endl;
    cout<<"	pushq %rbp"<<endl;			
    cout<<"	movq %rsp, %rbp"<<endl;
    
    BlockList::const_iterator it;
    for (it = blocks.begin(); it != blocks.end(); it++) {
      (**it).codeGen(context);
    }
    
    cout<<"	leave"<<endl;
    cout<<"	ret"<<endl;
  }
};


class NExpressionStatement : public NStatement {
 public:
  NExpression& expression;
 NExpressionStatement(NExpression& expression) :
  expression(expression) { }
  virtual void codeGen(CodeGenContext& context){return expression.codeGen(context);}
};


class NFunctionDeclaration : public NStatement {
 public:
  const NIdentifier& type;
  const NIdentifier& id;
  VariableList arguments;
  NBlock& block;
 NFunctionDeclaration(const NIdentifier& type, const NIdentifier& id,
		      const VariableList& arguments, NBlock& block) :
  type(type), id(id), arguments(arguments), block(block) { }
  virtual void codeGen(CodeGenContext& context){}
};

