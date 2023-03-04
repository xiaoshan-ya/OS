SECTION .data ;.data字段用于声明和初始化程序中的变量
message1 db 'please input your number: ', 0h ; dx指令为变量在内存中声明空间，并提供初始值,b为byte型变量大小
message2 db 'the sum of the numbers is: ', 0h
message3 db 'the product of the numbers is: ', 0h
message4 db 'you have an error in your input, please input again', 0h
SECTION .bss ;用于声明变量但不初始化
input resb 255; res用于为变量保留空间，但不赋值，32位计算机,int一共需要256byte
firstNum resb 255
secondNum resb 255
result resb 255
symbol resb 255

SECTION .text
global _start
; 计算字符串长度
length: ;eax原本为字符串初始地址，最后计算结果存在eax中
    push ebx
    mov ebx, eax ;将地址复制到ebx中，最后eax中的地址-ebx中的地址即为长度
nextChar:
    cmp byte [eax],0
    jz finish
    inc eax
    jmp nextChar
finish:    
    sub eax, ebx
    
    pop ebx
    ret
    

print:
    push edx
    push ecx
    push ebx
    push eax
    call length ;此时eax中是变量的长度
    mov edx, eax ;传入edx中
    pop eax ;原来存储的是输出变量的地址，需要恢复并存到ecx中
    mov ecx, eax ;ecx存储变量的地址（指向字符串的指针)
    mov ebx, 1
    mov eax, 4
    int 80h
    pop ebx
    pop ecx
    pop edx
    ret

;换行输出
println:
    call print
    push eax ;保存eax原有的值的地址
    mov eax, 0ah ;把换行符存入eax
    push eax ;保存eax中换行符
    mov eax, esp ;将栈顶元素传给eax
    call print
    pop eax
    pop eax
    ret
    
getline:
    ;实现输入,读取整行
    mov edx, 255
    mov ecx, input
    mov ebx, 0
    mov eax, 3
    int 80h
    
    push eax
    push ecx
    mov eax, input
    call length
    add ecx, eax
    dec ecx
    mov byte[ecx], 0h
    pop ecx
    pop eax
    ret
    
check:
    push eax
    push ecx
    push edx
    mov eax, input
    mov ecx, eax
    mov edx, eax
    call length
    add ecx, eax
    mov eax, edx
    dec eax
.loop:
    inc eax
    cmp eax, ecx
    jz .return
    cmp byte[eax], 42
    jz .loop
    cmp byte[eax], 43
    jz .loop
    cmp byte[eax], 48
    jz .loop
    cmp byte[eax], 49
    jz .loop
    cmp byte[eax], 50
    jz .loop
    cmp byte[eax], 51
    jz .loop
    cmp byte[eax], 52
    jz .loop
    cmp byte[eax], 53
    jz .loop
    cmp byte[eax], 54
    jz .loop
    cmp byte[eax], 55
    jz .loop
    cmp byte[eax], 56
    jz .loop
    cmp byte[eax], 57
    jz .loop
    
    mov ebx, 1
    jmp .return
.return:
    pop edx
    pop ecx
    pop eax
    ret            
    
getNum:
    mov ecx, input
    mov eax, input
    call length
    mov edx, eax
    mov eax, firstNum
    call cmpNumLoop
    
    mov eax, symbol
    push edx ;???????????????????????????后两行指令会改变edx的地址值
    mov dl, byte[ecx]
    mov byte[eax], dl
    pop edx
    inc ecx
    dec edx
    
    mov eax, secondNum
    call cmpNumLoop
cmpNumLoop:
    cmp edx, 0
    jz quitCmpNumLoop
    cmp byte[ecx], 42 ;取到当前ecx中地址的值，和*比较
    jz quitCmpNumLoop 
    cmp byte[ecx], 43 ;取到当前ecx中地址的值，和+比较
    jz quitCmpNumLoop
    push edx
    mov dl, byte[ecx]
    mov byte[eax], dl ;相当于eax为数组，改变数组中的值
    pop edx
    inc eax ;数组索引++
    inc ecx
    dec edx
    jmp cmpNumLoop
    
quitCmpNumLoop:
    ret
    
subCarry:
    push edx
    inc edx
    mov ecx, 1
    sub al, 10
    mov byte[edx], al
    pop edx
    jmp loopAdd

add:
    xor ecx, ecx ;作为进位寄存器，存放进位
    
    xor edx, edx
    mov edx, result
    add edx, 255 ;加法从后往前加，所以得到结果的顺序也是从后往前
    mov byte[edx], 0h
    dec edx
    
    mov eax, firstNum
    call length
    mov esi, eax
    add esi, firstNum
    sub esi, 1 ;得到最末尾的数
    mov eax, secondNum
    call length
    mov edi, eax
    add edi, secondNum
    sub edi, 1
    
loopAdd:
    cmp esi, firstNum
    jl secondRestAdd ;小于时跳转，当第二个数位数较大时，将额外的位数加入
    cmp edi, secondNum
    jl firstRestAdd
    xor eax, eax
    add al, byte[esi]
    sub al, 48 ;因为多加了一个ASCII码，且在寄存器中保存的数字也为ASCII码，所以下面不用再减一个
    add al, byte[edi]
    add al, cl
    xor ecx, ecx
    mov byte[edx], al
    
    dec esi ;向前走
    dec edi
    dec edx
    
    cmp al, 57 ;将低8位和9进行比较，超过则需要进1
    jg subCarry ;有符号大于则跳转
    jmp loopAdd
    

firstRestAdd:
    cmp esi, firstNum ;此时剩余firstnum，比较有没有加完，最后跳出循环
    jl finalAdd ;检查如果加到最后有没有进位剩余
    xor eax, eax
    add al, byte[esi]
    add al, cl
    xor ecx, ecx
    mov byte[edx], al
    
    dec esi
    dec edx
    
    cmp al, 57
    jg subCarry
    jmp firstRestAdd

secondRestAdd:
    cmp edi, secondNum
    jl finalAdd
    xor eax, eax
    add al, byte[edi]
    add al, cl
    xor ecx, ecx
    mov byte[edx], al
    
    dec edi
    dec edx
    
    cmp al, 57
    jg subCarry
    jmp secondRestAdd
    
    
finalAdd:
    cmp ecx, 1
    jnz printAddResult
    mov al, 48
    add al, cl
    xor ecx, ecx
    mov byte[edx], al
    dec edx
    jmp printAddResult
    
printAddResult:
    inc edx
    mov eax, message2
    call print
    mov eax, edx
    call println
    ret

;innerLoop函数
innerLoop:
    push ebx
    push esi
    push edx
.loop:    
    cmp esi, firstNum ;第一个数乘以edi,且esi是从后往前乘edi
    jl .return
    xor eax, eax
    xor ebx, ebx
    add al, byte[esi]
    sub al, 48
    add bl, byte[edi]
    sub bl, 48
    mul bl ;8位乘法，结果存放在AL中
    
    add byte[edx], al
    mov al, byte[edx]
    ;判断结果是否需要进位，ecx为进位器
    cmp al, 10
    jge .carry
    
    dec esi
    dec edx
    jmp .loop
.carry:
    mov bl, 10
    div bl ;被除数al， 除数bl，商al，余数ah
    mov byte[edx], ah
    dec edx
    add byte[edx], al
    
    dec esi
    jmp .loop
.return:
    pop edx
    pop esi
    pop ebx
    ret
                 
;format函数
format:
    push eax
    push ecx
    push edx
    push esi
    mov esi, edx ;esi存放result开头地址
    mov eax, result
    add eax, 255
    mov ecx, 0 ;1表示result没有全为0
.loop:
    cmp eax, edx ;此时edx在做完乘法在结果最前面，需要将edx中的数变成ASCII
    jz .judgezero ;有符号大于等于跳转
    add byte[edx], 48
    inc edx
    jmp .loop
    
.judgezero: ; 从开头移动eax
    mov eax, esi
    mov edx, esi
    call length
    add edx, eax
    mov eax, esi
    dec eax
.judgeloop:
    inc eax
    cmp eax, edx
    je .cmpzero
    cmp byte[eax], 48
    jne .judgeloop
    inc ecx
    jmp .judgeloop
    
 .cmpzero: ;从末尾移动eax
    mov eax, esi
    call length
    
    cmp ecx, eax
    jne .return
    
    mov edx, esi
    add edx, eax
    mov eax, edx
    mov edx, esi
    dec eax
.zeroloop:
    cmp edx, eax
    je .return
    mov byte[eax], 0h
    dec eax
    jmp .zeroloop
.return:
    pop esi
    pop edx
    pop ecx
    pop eax
    ret        
    
mul:
    xor ecx, ecx
    xor edx, edx
    mov edx, result
    xor ecx, ecx
    xor eax, eax
    add edx, 255
    mov byte[edx], 0h
    dec edx
    
    mov eax, firstNum
    call length
    mov esi, eax
    add esi, firstNum
    sub esi, 1 ;得到最末尾的数
    mov eax, secondNum
    call length
    mov edi, eax
    add edi, secondNum
    sub edi, 1
    
loopMul:
    cmp edi, secondNum ;对于乘法，如果第二个数乘完了，乘法也就结束了
    jl  .print
    call innerLoop
    dec edi
    dec edx
    jmp loopMul

.print:
    mov eax, message3
    call print
    
    ;eax中存放firstNum的长度
    mov eax, firstNum
    mov ecx, firstNum
    add ecx, 255
    mov byte[ecx], 0h
    call length
    sub eax, 2 ;将edx移到result的第一位
    sub edx, eax
    sub edx, 1
    cmp byte[edx], 0
    jnz .carryPrint    ;如果最高位有进位，则edx需要再往前一位
    inc edx
    call format
    mov eax, edx
    call println
    ret       
.carryPrint:
    call format
    mov eax, edx
    call println
    ret
              
clean:
    push eax
    push ebx
    mov ebx, eax
    add eax, 255
.loop:    
    cmp eax, ebx
    jb .return ;小于跳转
    mov byte[eax], 0
    dec eax
    jmp .loop
.return:
   pop ebx
   pop eax
   ret
   
   
_start:
    mov ebp, esp; for correct debugging
    ;先将寄存器清零
    mov eax, symbol         ;
    call clean
    mov eax, input          ;
    call clean
    mov eax, result         ;
    call clean
    mov eax, firstNum       ;
    call clean
    mov eax, secondNum      ;
    call clean
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov ebp, esp; for correct debugging

.loop:        
    mov eax, message1
    call print
    call getline
    ;遇到q跳出  
    mov eax, input
    cmp byte[eax], 113
    jz .quit
    
    ;检查输入错误
    mov ebx, 0 ;0时符合输入，1时有错误输入
    call check
    cmp ebx, 1
    jz .error
    
    call getNum ;firstNum里面保存的是ASCII形式的数据
    
    mov eax, symbol
    cmp byte[eax], 43 ;加法运算
    jz .addCal ;相等才跳转
    
    cmp byte[eax], 42
    jz .mulCal
    
.quit:
    mov ebx, 0
    mov eax, 1
    int 80h
    
.error:
    mov eax, input
    call println
    
    mov eax, message4
    call println
    jmp _start
    
.addCal:
    call add
    jmp _start
    
.mulCal:
    call mul
    jmp _start
    

