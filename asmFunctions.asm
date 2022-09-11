;MIT License
;Copyright (c) 2022 Jared Loewenthal
;
;Permission is hereby granted, free of charge, to any person obtaining a copy
;of this software and associated documentation files (the "Software"), to deal
;in the Software without restriction, including without limitation the rights
;to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;copies of the Software, and to permit persons to whom the Software is
;furnished to do so, subject to the following conditions:
;
;The above copyright notice and this permission notice shall be included in all
;copies or substantial portions of the Software.
;
;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;SOFTWARE.

;x64 Assembly Code with Functions to be called by the main program
;Utilizes AVX2 and FMA instructions (needs a modernish CPU)
;Assembled by Flat Assembler (FASM) 
;Initial Version 0.1

format ELF64 ;This format is compatible with gcc / ld
;Uses Microsoft x64 Calling Convention
;Does Not "Push" or "Pop" Non-Volatile XMM registers

public inData16ToWindowData
inData16ToWindowData:
;rax: return value, rcx: inDataPtr, rdx: adjHannWindowPtr, r8: windowDataNextPtr, r9: windowDataNowPtr
	
	mov r10, rdx
	add r10, 0x780
	
	mov eax, 15
	.innerLoop:
		
		vpmovsxwd ymm0, [rcx + 0x00] 
		vpmovsxwd ymm1, [rcx + 0x10]
		vpmovsxwd ymm2, [rcx + 0x20]
		vpmovsxwd ymm3, [rcx + 0x30]
		add rcx, 0x40
		
		vcvtdq2ps ymm0, ymm0
		vcvtdq2ps ymm1, ymm1
		vcvtdq2ps ymm2, ymm2
		vcvtdq2ps ymm3, ymm3
		
		vmulps ymm4, ymm0, [rdx + 0x00]
		vmulps ymm5, ymm1, [rdx + 0x20]
		vmulps ymm6, ymm2, [rdx + 0x40]
		vmulps ymm7, ymm3, [rdx + 0x60]
		add rdx, 0x80
		
		vmovaps [r8 + 0x00], ymm4
		vmovaps [r8 + 0x20], ymm5
		vmovaps [r8 + 0x40], ymm6
		vmovaps [r8 + 0x60], ymm7
		add r8, 0x80
		
		vmulps ymm8, ymm0, [r10 + 0x00]
		vmulps ymm9, ymm1, [r10 + 0x20]
		vmulps ymm10,ymm2, [r10 + 0x40]
		vmulps ymm11,ymm3, [r10 + 0x60]
		add r10, 0x80
		
		vmovaps [r9 + 0x00], ymm8
		vmovaps [r9 + 0x20], ymm9
		vmovaps [r9 + 0x40], ymm10
		vmovaps [r9 + 0x60], ymm11
		add r9, 0x80
		
		dec eax
	jnz .innerLoop
	
ret ;End of inData16ToWindowData


public windowDataToOutData16
windowDataToOutData16:
;rax: return value, rcx: windowDataInvPtr, rdx: adjHannWindowInvPtr, r8: windowDataAccumPtr, r9: outDataPtr

	mov r10, r8
	
	mov eax, 15
	.innerLoop1:
		
		vmovaps ymm0, [rcx + 0x00] 
		vmovaps ymm1, [rcx + 0x20]
		vmovaps ymm2, [rcx + 0x40]
		vmovaps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vmovaps ymm4, [rdx + 0x00]
		vmovaps ymm5, [rdx + 0x20]
		vmovaps ymm6, [rdx + 0x40]
		vmovaps ymm7, [rdx + 0x60]
		add rdx, 0x80
		
		vfmadd213ps ymm0, ymm4, [r8 + 0x00]
		vfmadd213ps ymm1, ymm5, [r8 + 0x20]
		vfmadd213ps ymm2, ymm6, [r8 + 0x40]
		vfmadd213ps ymm3, ymm7, [r8 + 0x60]
		add r8, 0x80
		
		vroundps ymm0, ymm0, 00b
		vroundps ymm1, ymm1, 00b
		vroundps ymm2, ymm2, 00b
		vroundps ymm3, ymm3, 00b
		
		vcvtps2dq ymm0, ymm0
		vcvtps2dq ymm1, ymm1
		vcvtps2dq ymm2, ymm2
		vcvtps2dq ymm3, ymm3
		
		vpackssdw ymm8, ymm0, ymm1
		vpackssdw ymm9, ymm2, ymm3
		
		vpermq ymm8, ymm8, 11011000b
		vpermq ymm9, ymm9, 11011000b
		
		vmovaps [r9 + 0x00], ymm8
		vmovaps [r9 + 0x20], ymm9
		add r9, 0x40
		
		dec eax
	jnz .innerLoop1
	
	mov eax, 15
	.innerLoop2:
		
		vmovaps ymm0, [rcx + 0x00] 
		vmovaps ymm1, [rcx + 0x20]
		vmovaps ymm2, [rcx + 0x40]
		vmovaps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vmulps ymm0, ymm0, [rdx + 0x00]
		vmulps ymm1, ymm1, [rdx + 0x20]
		vmulps ymm2, ymm2, [rdx + 0x40]
		vmulps ymm3, ymm3, [rdx + 0x60]
		add rdx, 0x80
		
		vmovaps [r10 + 0x00], ymm0
		vmovaps [r10 + 0x20], ymm1
		vmovaps [r10 + 0x40], ymm2
		vmovaps [r10 + 0x60], ymm3
		add r10, 0x80
		
		dec eax
	jnz .innerLoop2
	
ret ;End of windowDataToOutData16


public inData32ToWindowData
inData32ToWindowData:
;rax: return value, rcx: inDataPtr, rdx: adjHannWindowPtr, r8: windowDataNextPtr, r9: windowDataNowPtr
	
	mov r10, rdx
	add r10, 0x780
	
	mov eax, 15
	.innerLoop:
		
		vcvtdq2ps ymm0, [rcx + 0x00] 
		vcvtdq2ps ymm1, [rcx + 0x20]
		vcvtdq2ps ymm2, [rcx + 0x40]
		vcvtdq2ps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vmulps ymm4, ymm0, [rdx + 0x00]
		vmulps ymm5, ymm1, [rdx + 0x20]
		vmulps ymm6, ymm2, [rdx + 0x40]
		vmulps ymm7, ymm3, [rdx + 0x60]
		add rdx, 0x80
		
		vmovaps [r8 + 0x00], ymm4
		vmovaps [r8 + 0x20], ymm5
		vmovaps [r8 + 0x40], ymm6
		vmovaps [r8 + 0x60], ymm7
		add r8, 0x80
		
		vmulps ymm8, ymm0, [r10 + 0x00]
		vmulps ymm9, ymm1, [r10 + 0x20]
		vmulps ymm10,ymm2, [r10 + 0x40]
		vmulps ymm11,ymm3, [r10 + 0x60]
		add r10, 0x80
		
		vmovaps [r9 + 0x00], ymm8
		vmovaps [r9 + 0x20], ymm9
		vmovaps [r9 + 0x40], ymm10
		vmovaps [r9 + 0x60], ymm11
		add r9, 0x80
		
		dec eax
	jnz .innerLoop
	
ret ;End of inData32ToWindowData


public windowDataToOutData32
windowDataToOutData32:
;rax: return value, rcx: windowDataInvPtr, rdx: adjHannWindowInvPtr, r8: windowDataAccumPtr, r9: outDataPtr
	
	mov r10, r8
	
	mov eax, 15
	.innerLoop1:
		
		vmovaps ymm0, [rcx + 0x00] 
		vmovaps ymm1, [rcx + 0x20]
		vmovaps ymm2, [rcx + 0x40]
		vmovaps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vmovaps ymm4, [rdx + 0x00]
		vmovaps ymm5, [rdx + 0x20]
		vmovaps ymm6, [rdx + 0x40]
		vmovaps ymm7, [rdx + 0x60]
		add rdx, 0x80
		
		vfmadd213ps ymm0, ymm4, [r8 + 0x00]
		vfmadd213ps ymm1, ymm5, [r8 + 0x20]
		vfmadd213ps ymm2, ymm6, [r8 + 0x40]
		vfmadd213ps ymm3, ymm7, [r8 + 0x60]
		add r8, 0x80
		
		vroundps ymm0, ymm0, 00b
		vroundps ymm1, ymm1, 00b
		vroundps ymm2, ymm2, 00b
		vroundps ymm3, ymm3, 00b
		
		vcvtps2dq ymm0, ymm0
		vcvtps2dq ymm1, ymm1
		vcvtps2dq ymm2, ymm2
		vcvtps2dq ymm3, ymm3
		
		vmovaps [r9 + 0x00], ymm0
		vmovaps [r9 + 0x20], ymm1
		vmovaps [r9 + 0x40], ymm2
		vmovaps [r9 + 0x60], ymm3
		add r9, 0x80
		
		dec eax
	jnz .innerLoop1
	
	mov eax, 15
	.innerLoop2:
		
		vmovaps ymm0, [rcx + 0x00] 
		vmovaps ymm1, [rcx + 0x20]
		vmovaps ymm2, [rcx + 0x40]
		vmovaps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vmulps ymm0, ymm0, [rdx + 0x00]
		vmulps ymm1, ymm1, [rdx + 0x20]
		vmulps ymm2, ymm2, [rdx + 0x40]
		vmulps ymm3, ymm3, [rdx + 0x60]
		add rdx, 0x80
		
		vmovaps [r10 + 0x00], ymm0
		vmovaps [r10 + 0x20], ymm1
		vmovaps [r10 + 0x40], ymm2
		vmovaps [r10 + 0x60], ymm3
		add r10, 0x80
		
		dec eax
	jnz .innerLoop2
	
ret ;End of windowDataToOutData32


public computeMAT5Max0asm
computeMAT5Max0asm: ;rax is return value and rcx is a pointer to the parameters	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, rcx ;Iteration Integer for loop
	;mov rdx, [rcx + 0x08] ;Matrix data pointer which includes initalization data
	;mov r8,  [rcx + 0x10] ;Input float array for broadcasting
	;mov r9,  [rcx + 0x18]	;Output data pointer
	
	mov r10, 0x0A0 ;Helper register for matrix data pointer increment (r10d?)
	
	;mov aligned (32 bytes) 8-floats initalization data into ymm1-ymm15
	vmovaps ymm1,  [rdx + 0x000] 
	vmovaps ymm2,  [rdx + 0x020]
	vmovaps ymm3,  [rdx + 0x040]
	vmovaps ymm4,  [rdx + 0x060]
	vmovaps ymm5,  [rdx + 0x080]
	
	add rdx, r10
	
	.innerLoop:
		
		vbroadcastss ymm0, [r8]
		add r8, 0x4 ; adding 4 bytes for a float size
		
		vfmadd231ps ymm1,   ymm0, [rdx + 0x000]
		vfmadd231ps ymm2,   ymm0, [rdx + 0x020]
		vfmadd231ps ymm3,   ymm0, [rdx + 0x040]
		vfmadd231ps ymm4,   ymm0, [rdx + 0x060]
		vfmadd231ps ymm5,   ymm0, [rdx + 0x080]
		add rdx, r10
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
	
	vxorps ymm0, ymm0, ymm0 ; Set ymm0 to zero
	
	;vmovaps instead of vmovaps for writes?
	vmaxps ymm1, ymm1, ymm0
	vmaxps ymm2, ymm2, ymm0
	vmovaps [r9 + 0x00], ymm1
	
	vmaxps ymm3, ymm3, ymm0
	vmovaps [r9 + 0x20], ymm2
	
	vmaxps ymm4, ymm4, ymm0
	vmovaps [r9 + 0x40], ymm3
	
	vmaxps ymm5, ymm5, ymm0
	vmovaps [r9 + 0x60], ymm4
	
	vmovaps [r9 + 0x80], ymm5
	
ret ;End of computeMAT5Max0asm


public computeMAT15GRUasm
computeMAT15GRUasm: ;rax is return value and rcx is a pointer to the parameters	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, rcx ;Iteration Integer for loop
	;mov rdx, [rcx + 0x08] ;Matrix data pointer which includes initalization data
	;mov r8,  [rcx + 0x10] ;Input float array for broadcasting
	;mov r9,  [rcx + 0x18]	;Output data pointer
	
	mov r10, 0x1E0 ;Helper register for matrix data pointer increment (r10d?)
	
	;mov aligned (32 bytes) 8-floats initalization data into ymm1-ymm15
	vmovaps ymm1,  [r9 + 0x000] 
	vmovaps ymm2,  [r9 + 0x020]
	vmovaps ymm3,  [r9 + 0x040]
	vmovaps ymm4,  [r9 + 0x060]
	vmovaps ymm5,  [r9 + 0x080]
	vmovaps ymm6,  [r9 + 0x0A0]
	vmovaps ymm7,  [r9 + 0x0C0]
	vmovaps ymm8,  [r9 + 0x0E0]
	vmovaps ymm9,  [r9 + 0x100]
	vmovaps ymm10, [r9 + 0x120]
	vmovaps ymm11, [r9 + 0x140]
	vmovaps ymm12, [r9 + 0x160]
	vmovaps ymm13, [r9 + 0x180]
	vmovaps ymm14, [r9 + 0x1A0]
	vmovaps ymm15, [r9 + 0x1C0]
	
	.innerLoop:
		
		vbroadcastss ymm0, [r8]
		add r8, 0x4 ; adding 4 bytes for a float size
		
		vfmadd231ps ymm1,   ymm0, [rdx + 0x000]
		vfmadd231ps ymm2,   ymm0, [rdx + 0x020]
		vfmadd231ps ymm3,   ymm0, [rdx + 0x040]
		vfmadd231ps ymm4,   ymm0, [rdx + 0x060]
		vfmadd231ps ymm5,   ymm0, [rdx + 0x080]
		vfmadd231ps ymm6,   ymm0, [rdx + 0x0A0]
		vfmadd231ps ymm7,   ymm0, [rdx + 0x0C0]
		vfmadd231ps ymm8,   ymm0, [rdx + 0x0E0]
		vfmadd231ps ymm9,   ymm0, [rdx + 0x100]
		vfmadd231ps ymm10,  ymm0, [rdx + 0x120]
		vfmadd231ps ymm11,  ymm0, [rdx + 0x140]
		vfmadd231ps ymm12,  ymm0, [rdx + 0x160]
		vfmadd231ps ymm13,  ymm0, [rdx + 0x180]
		vfmadd231ps ymm14,  ymm0, [rdx + 0x1A0]
		vfmadd231ps ymm15,  ymm0, [rdx + 0x1C0]
		add rdx, r10
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
	
	vmovaps [r9 + 0x00], ymm1
	vmovaps [r9 + 0x20], ymm2
	vmovaps [r9 + 0x40], ymm3
	vmovaps [r9 + 0x60], ymm4
	vmovaps [r9 + 0x80], ymm5
	vmovaps [r9 + 0xA0], ymm6
	vmovaps [r9 + 0xC0], ymm7
	vmovaps [r9 + 0xE0], ymm8
	vmovaps [r9 + 0x100], ymm9
	vmovaps [r9 + 0x120], ymm10
	vmovaps [r9 + 0x140], ymm11
	vmovaps [r9 + 0x160], ymm12
	vmovaps [r9 + 0x180], ymm13
	vmovaps [r9 + 0x1A0], ymm14
	vmovaps [r9 + 0x1C0], ymm15
	
ret ;End of computeMAT15GRUasm


public computeMAT15GRUasm2
computeMAT15GRUasm2: ;rax is return value and rcx is a pointer to the parameters	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, rcx ;Iteration Integer for loop
	;mov rdx, [rcx + 0x08] ;Matrix data pointer which includes initalization data
	;mov r8,  [rcx + 0x10] ;Input float array for broadcasting
	;mov r9,  [rcx + 0x18]	;Output data pointer
	
	mov r10, 0x1E0 ;Helper register for matrix data pointer increment (r10d?)
	
	;mov aligned (32 bytes) 8-floats initalization data into ymm1-ymm15
	vmovaps ymm1,  [rdx + 0x000] 
	vmovaps ymm2,  [rdx + 0x020]
	vmovaps ymm3,  [rdx + 0x040]
	vmovaps ymm4,  [rdx + 0x060]
	vmovaps ymm5,  [rdx + 0x080]
	vmovaps ymm6,  [rdx + 0x0A0]
	vmovaps ymm7,  [rdx + 0x0C0]
	vmovaps ymm8,  [rdx + 0x0E0]
	vmovaps ymm9,  [rdx + 0x100]
	vmovaps ymm10, [rdx + 0x120]
	vmovaps ymm11, [rdx + 0x140]
	vmovaps ymm12, [rdx + 0x160]
	vmovaps ymm13, [rdx + 0x180]
	vmovaps ymm14, [rdx + 0x1A0]
	vmovaps ymm15, [rdx + 0x1C0]
	
	add rdx, r10
	
	.innerLoop:
		
		vbroadcastss ymm0, [r8]
		add r8, 0x4 ; adding 4 bytes for a float size
		
		vfmadd231ps ymm1,   ymm0, [rdx + 0x000]
		vfmadd231ps ymm2,   ymm0, [rdx + 0x020]
		vfmadd231ps ymm3,   ymm0, [rdx + 0x040]
		vfmadd231ps ymm4,   ymm0, [rdx + 0x060]
		vfmadd231ps ymm5,   ymm0, [rdx + 0x080]
		vfmadd231ps ymm6,   ymm0, [rdx + 0x0A0]
		vfmadd231ps ymm7,   ymm0, [rdx + 0x0C0]
		vfmadd231ps ymm8,   ymm0, [rdx + 0x0E0]
		vfmadd231ps ymm9,   ymm0, [rdx + 0x100]
		vfmadd231ps ymm10,  ymm0, [rdx + 0x120]
		vfmadd231ps ymm11,  ymm0, [rdx + 0x140]
		vfmadd231ps ymm12,  ymm0, [rdx + 0x160]
		vfmadd231ps ymm13,  ymm0, [rdx + 0x180]
		vfmadd231ps ymm14,  ymm0, [rdx + 0x1A0]
		vfmadd231ps ymm15,  ymm0, [rdx + 0x1C0]
		add rdx, r10
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
	
	vmovaps [r9 + 0x00], ymm1
	vmovaps [r9 + 0x20], ymm2
	vmovaps [r9 + 0x40], ymm3
	vmovaps [r9 + 0x60], ymm4
	vmovaps [r9 + 0x80], ymm5
	vmovaps [r9 + 0xA0], ymm6
	vmovaps [r9 + 0xC0], ymm7
	vmovaps [r9 + 0xE0], ymm8
	vmovaps [r9 + 0x100], ymm9
	vmovaps [r9 + 0x120], ymm10
	vmovaps [r9 + 0x140], ymm11
	vmovaps [r9 + 0x160], ymm12
	vmovaps [r9 + 0x180], ymm13
	vmovaps [r9 + 0x1A0], ymm14
	vmovaps [r9 + 0x1C0], ymm15
	
ret ;End of computeMAT15GRUasm2


public computeMAT8NoMaxasm
computeMAT8NoMaxasm: ;rax is return value and rcx is a pointer to the parameters	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, rcx ;Iteration Integer for loop
	;mov rdx, [rcx + 0x08] ;Matrix data pointer which includes initalization data
	;mov r8,  [rcx + 0x10] ;Input float array for broadcasting
	;mov r9,  [rcx + 0x18]	;Output data pointer
	
	mov r10, 0x100 ;Helper register for matrix data pointer increment (r10d?)
	
	;mov aligned (32 bytes) 8-floats initalization data into ymm1-ymm15
	vmovaps ymm1,  [rdx + 0x000] 
	vmovaps ymm2,  [rdx + 0x020]
	vmovaps ymm3,  [rdx + 0x040]
	vmovaps ymm4,  [rdx + 0x060]
	vmovaps ymm5,  [rdx + 0x080]
	vmovaps ymm6,  [rdx + 0x0A0]
	vmovaps ymm7,  [rdx + 0x0C0]
	vmovaps ymm8,  [rdx + 0x0E0]
	
	add rdx, r10
	
	.innerLoop:
		
		vbroadcastss ymm0, [r8]
		add r8, 0x4 ; adding 4 bytes for a float size
		
		vfmadd231ps ymm1,   ymm0, [rdx + 0x000]
		vfmadd231ps ymm2,   ymm0, [rdx + 0x020]
		vfmadd231ps ymm3,   ymm0, [rdx + 0x040]
		vfmadd231ps ymm4,   ymm0, [rdx + 0x060]
		vfmadd231ps ymm5,   ymm0, [rdx + 0x080]
		vfmadd231ps ymm6,   ymm0, [rdx + 0x0A0]
		vfmadd231ps ymm7,   ymm0, [rdx + 0x0C0]
		vfmadd231ps ymm8,   ymm0, [rdx + 0x0E0]
		add rdx, r10
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
	
	vmovaps [r9 + 0x00], ymm1
	vmovaps [r9 + 0x20], ymm2
	vmovaps [r9 + 0x40], ymm3
	vmovaps [r9 + 0x60], ymm4
	vmovaps [r9 + 0x80], ymm5
	vmovaps [r9 + 0xA0], ymm6
	vmovaps [r9 + 0xC0], ymm7
	vmovaps [r9 + 0xE0], ymm8
	
ret ;End of computeMAT8NoMaxasm


public computeMAT5NoMaxasm
computeMAT5NoMaxasm: ;rax is return value and rcx is a pointer to the parameters	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, rcx ;Iteration Integer for loop
	;mov rdx, [rcx + 0x08] ;Matrix data pointer which includes initalization data
	;mov r8,  [rcx + 0x10] ;Input float array for broadcasting
	;mov r9,  [rcx + 0x18]	;Output data pointer
	
	mov r10, 0x0A0 ;Helper register for matrix data pointer increment (r10d?)
	
	;mov aligned (32 bytes) 8-floats initalization data into ymm1-ymm15
	vmovaps ymm1,  [rdx + 0x000] 
	vmovaps ymm2,  [rdx + 0x020]
	vmovaps ymm3,  [rdx + 0x040]
	vmovaps ymm4,  [rdx + 0x060]
	vmovaps ymm5,  [rdx + 0x080]
	
	add rdx, r10
	
	.innerLoop:
		
		vbroadcastss ymm0, [r8]
		add r8, 0x4 ; adding 4 bytes for a float size
		
		vfmadd231ps ymm1,   ymm0, [rdx + 0x000]
		vfmadd231ps ymm2,   ymm0, [rdx + 0x020]
		vfmadd231ps ymm3,   ymm0, [rdx + 0x040]
		vfmadd231ps ymm4,   ymm0, [rdx + 0x060]
		vfmadd231ps ymm5,   ymm0, [rdx + 0x080]
		add rdx, r10
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
		
	vmovaps [r9 + 0x00], ymm1
	vmovaps [r9 + 0x20], ymm2
	vmovaps [r9 + 0x40], ymm3
	vmovaps [r9 + 0x60], ymm4
	vmovaps [r9 + 0x80], ymm5
	
ret ;End of computeMAT5NoMaxasm


public computeGRUasm32
computeGRUasm32: ;rax is return value and rcx is a pointer to the parameters
	;parameters are either an address to the start of a float array or an integer (seperated by 8 bytes)
	
	;Load the parameters into volatile registers (removing need for other pushes and pops)
	mov rax, [rcx + 0x00] ;Iteration Integer for loop
	mov rdx, [rcx + 0x30] ;Output Data / Ht1
	mov r8,  [rcx + 0x38] ;Exp Constants Float Array
	mov r9,  [rcx + 0x18] ;Ht2
	mov r10, [rcx + 0x20] ;Rt
	mov r11, [rcx + 0x28] ;Zt
	
	vbroadcastss ymm1, [r8 + 0x00] ;-log2(e)
	vbroadcastss ymm2, [r8 + 0x04] ;-ln(2) high
	vbroadcastss ymm3, [r8 + 0x08] ;-ln(2) low
	vbroadcastss ymm4, [r8 + 0x0C] ;c0
	vbroadcastss ymm5, [r8 + 0x10] ;c1
	vbroadcastss ymm6, [r8 + 0x14] ;c2
	vbroadcastss ymm7, [r8 + 0x18] ;c3
	vbroadcastss ymm8, [r8 + 0x1C] ;c4
	vbroadcastss ymm0, [r8 + 0x20] ;c5 Should contain +1.0(f)
	vbroadcastss ymm9, [r8 + 0x24] ;0x00800000 for integer add / * 2
	vbroadcastss ymm10,[r8 + 0x28] ;minimum ~-84 for integer add / * 2
	
	mov r8, [rcx + 0x40] ;GRU Hidden State Byte Offset
	
	.innerLoop3:
		
		; e^-Rt:
		vmovaps ymm11, [r10] ; Load Rt into ymm11
		
		vmaxps ymm11, ymm11, ymm10 ;Input range check
		
		vmulps ymm12, ymm1, ymm11
		vroundps ymm13, ymm12, 00b;{rn-sae} ;round to nearest
		
		vfmsub231ps ymm11, ymm2, ymm13
		vfmadd231ps ymm11, ymm3, ymm13
		
		vcvtps2dq ymm12, ymm12
		vpslld ymm12, ymm12, 23 ;mantissa length
		
		vmovaps ymm14, ymm4
		vfmadd213ps ymm14, ymm11, ymm5
		vfmadd213ps ymm14, ymm11, ymm6
		vfmadd213ps ymm14, ymm11, ymm7
		vfmadd213ps ymm14, ymm11, ymm8
		vfmadd213ps ymm14, ymm11, ymm0
		vfmadd213ps ymm14, ymm11, ymm0 ;extra polynomial precision
		
		vpaddd ymm11, ymm12, ymm14
		
		; sigmoid(Rt):
		vaddps ymm11, ymm0, ymm11
		vdivps ymm11, ymm0, ymm11
		
		; calculate Ht (into ymm11):
		vmulps ymm11, ymm11, [rdx] ; multiply by stored Ht1
		vaddps ymm11, ymm11, [r9] ; add with stored Ht2
		
		; e^(-2*Ht):
		vpaddd ymm11, ymm11, ymm9
		
		vmaxps ymm11, ymm11, ymm10 ;Input range check
		
		vmulps ymm12, ymm1, ymm11
		vroundps ymm13, ymm12, 00b;{rn-sae} ;round to nearest
		
		vfmsub231ps ymm11, ymm2, ymm13
		vfmadd231ps ymm11, ymm3, ymm13
		
		vcvtps2dq ymm12, ymm12
		vpslld ymm12, ymm12, 23 ;mantissa length
		
		vmovaps ymm14, ymm4
		vfmadd213ps ymm14, ymm11, ymm5
		vfmadd213ps ymm14, ymm11, ymm6
		vfmadd213ps ymm14, ymm11, ymm7
		vfmadd213ps ymm14, ymm11, ymm8
		vfmadd213ps ymm14, ymm11, ymm0
		vfmadd213ps ymm14, ymm11, ymm0 ;extra polynomial precision
		
		vpaddd ymm11, ymm12, ymm14
		
		; tanh(Ht):
		vaddps ymm15, ymm0, ymm11
		vsubps ymm14, ymm0, ymm11
		vdivps ymm15, ymm14, ymm15
		
		; e^-Zt:
		vmovaps ymm11, [r11] ; Load Zt into ymm11
		
		vmaxps ymm11, ymm11, ymm10 ;Input range check
		
		vmulps ymm12, ymm1, ymm11
		vroundps ymm13, ymm12, 00b;{rn-sae} ;round to nearest
		
		vfmsub231ps ymm11, ymm2, ymm13
		vfmadd231ps ymm11, ymm3, ymm13
		
		vcvtps2dq ymm12, ymm12
		vpslld ymm12, ymm12, 23 ;mantissa length
		
		vmovaps ymm14, ymm4
		vfmadd213ps ymm14, ymm11, ymm5
		vfmadd213ps ymm14, ymm11, ymm6
		vfmadd213ps ymm14, ymm11, ymm7
		vfmadd213ps ymm14, ymm11, ymm8
		vfmadd213ps ymm14, ymm11, ymm0
		vfmadd213ps ymm14, ymm11, ymm0 ;extra polynomial precision
		
		vpaddd ymm11, ymm12, ymm14
		
		;ymm11 is e^-Zt here, ymm15 is running result
		; sigmoid(Zt):
		vaddps ymm12, ymm0, ymm11
		vdivps ymm13, ymm0, ymm12 ;ymm13 is zt
		vdivps ymm14, ymm11, ymm12 ;ymm14 is more accurate 1-zt at cost of another division
		
		; newState calculation
		vmulps ymm11, ymm13, [r8] ;multiply zt by appropriate values of previous state
		vfmadd213ps ymm14, ymm15, ymm11
		;vmulps ymm14, ymm12, ymm15
		;vaddps ymm11, ymm13, ymm14
		
		; store result calculation into output
		vmovaps [rdx], ymm14 ;ymm11
		
		add rdx, 0x20 ;Output Pointer / Ht1
		add r8, 0x20 ;ht-1
		add r9, 0x20 ;Ht2
		add r10, 0x20 ;Rt
		add r11, 0x20 ;Zt
		
		dec eax
	jnz .innerLoop3
	
ret ;End of computeGRUasm32


public computeMAT1asm
computeMAT1asm: ;rax: return value, rcx: matrixPtr, rdx: inputPtr, r8: iterations, r9: outputPtr
	
	;set ymm4-ymm7 to zero
	vxorps ymm4, ymm4, ymm4
	vxorps ymm5, ymm5, ymm5
	vxorps ymm6, ymm6, ymm6
	vxorps ymm7, ymm7, ymm7
	
	mov rax, r8
	.innerLoop:
		
		vmovaps ymm0, [rcx + 0x00] 
		vmovaps ymm1, [rcx + 0x20]
		vmovaps ymm2, [rcx + 0x40]
		vmovaps ymm3, [rcx + 0x60]
		add rcx, 0x80
		
		vfmadd231ps ymm4,  ymm0, [rdx + 0x00]
		vfmadd231ps ymm5,  ymm1, [rdx + 0x20]
		vfmadd231ps ymm6,  ymm2, [rdx + 0x40]
		vfmadd231ps ymm7,  ymm3, [rdx + 0x60]
		add rdx, 0x80
		
		dec eax ;Minimal code size for 1 subtraction (assumes loop iterator won't be more than 4 bytes)
	jnz .innerLoop
	
	vaddps ymm4, ymm4, ymm5
	vaddps ymm6, ymm6, ymm7
	
	vmovss xmm0, [rcx]
	
	vaddps ymm4, ymm4, ymm6
	
	vextractf128 xmm5, ymm4, 0x1
	vaddps xmm4, xmm5, xmm4
	
	vmovshdup xmm5, xmm4
	vaddps xmm4, xmm5, xmm4
	vmovhlps xmm5, xmm5, xmm4
	vaddss xmm4, xmm4, xmm5
	vaddss xmm0, xmm0, xmm4
	
	vmovss [r9], xmm0
	
ret ;End of computeMAT1asm


public absClampLog
absClampLog:
;rax: return value, rcx: fftDataPtr, rdx: fftAbsPtr, r8: logConstantPtr, r9: ???
	
	vbroadcastss ymm0, [r8 + 0x00] ;+1.0f
	vbroadcastss ymm1, [r8 + 0x04] ;mantissaMSb
	vbroadcastss ymm2, [r8 + 0x08] ;exponent
	vbroadcastss ymm3, [r8 + 0x0C] ;mantissa
	vbroadcastss ymm4, [r8 + 0x10] ;A
	vbroadcastss ymm5, [r8 + 0x14] ;B
	vbroadcastss ymm6, [r8 + 0x18] ;C
	vbroadcastss ymm7, [r8 + 0x1C] ;D
	vbroadcastss ymm8, [r8 + 0x20] ;E
	vbroadcastss ymm9, [r8 + 0x24] ;log10of2L (low translate)
	vbroadcastss ymm10,[r8 + 0x28] ;log10of2H (high translate)
	vbroadcastss ymm11,[r8 + 0x2C] ;clamp value
	
	mov r10, rcx
	add r10, 0x820 ;halfway
	
	mov eax, 65 ;ceil(513/8)
	
	.innerLoop:
		
		vmovaps ymm12, [rcx] ;Load real value
		vmovaps ymm13, [r10] ;Load imag value
		add rcx, 0x20
		add r10, 0x20
		
		vmulps ymm12, ymm12, ymm12
		vfmadd231ps ymm12, ymm13, ymm13
		;ymm12 now has abs of fftData (real * real) + (imag * imag)
		
		vmaxps ymm12, ymm12, ymm11 ;Clamp ymm12
		
		vmovaps ymm13, ymm0 ;ymm13 = bias ...better mov ?
		vpand ymm14, ymm12, ymm1 ;ymm14 = mantissaMSb
		vpand ymm15, ymm12, ymm3 ;ymm15 = mantissa
		
		vpslld ymm14, ymm14, 1
		vpsubd ymm13, ymm13, ymm14		
		vpand ymm13, ymm13, ymm0
		vpor ymm13, ymm13, ymm15
		
		vpand ymm15, ymm12, ymm2 ;ymm15 = exponent
		vsubps ymm13, ymm13, ymm0
		vpaddd ymm15, ymm15, ymm14
		vmovaps ymm12, ymm4
		vpsubd ymm15, ymm15, ymm0
		
		vfmadd213ps ymm12, ymm13, ymm5
		vaddps ymm14, ymm13, ymm7
		vpsrad ymm15, ymm15, 23
		
		vfmadd213ps ymm12, ymm13, ymm6
		vfmadd213ps ymm14, ymm13, ymm8
		vcvtdq2ps ymm15, ymm15
		
		vmulps ymm12, ymm12, ymm13
		vdivps ymm12, ymm12, ymm14
		vaddps ymm12, ymm12, ymm15
		
		vmulps ymm13, ymm12, ymm9
		vfmadd231ps ymm13, ymm12, ymm10
		
		vmovaps [rdx], ymm13 ;Store fftAbs		
		add rdx, 0x20
		
		dec eax
	jnz .innerLoop
	
ret ;End of absClampLog


public sigmoidClampMultiply
sigmoidClampMultiply: ;rax: return value, rcx: mat4OutputPtr, rdx: fftDataPtr, r8: expConstantPtr, r9: ???
	
	vbroadcastss ymm1, [r8 + 0x00] ;-log2(e)
	vbroadcastss ymm2, [r8 + 0x04] ;-ln(2) high
	vbroadcastss ymm3, [r8 + 0x08] ;-ln(2) low
	vbroadcastss ymm4, [r8 + 0x0C] ;c0
	vbroadcastss ymm5, [r8 + 0x10] ;c1
	vbroadcastss ymm6, [r8 + 0x14] ;c2
	vbroadcastss ymm7, [r8 + 0x18] ;c3
	vbroadcastss ymm8, [r8 + 0x1C] ;c4
	vbroadcastss ymm0, [r8 + 0x20] ;c5 Should contain +1.0(f)
	vbroadcastss ymm9, [r8 + 0x28] ;minimum ~-84 for integer add / * 2
	vbroadcastss ymm10,[r8 + 0x2C] ;clamp value
	
	mov r10, rdx
	add r10, 0x820
	
	mov eax, 65 ;ceil(513/8)
	
	.innerLoop:
		
		vmovaps ymm11, [rcx] ;Load value
		add rcx, 0x20
		
		vmaxps ymm11, ymm11, ymm9 ;Input range check (needed?)
		
		vmovaps ymm14, [rdx] ;Load fftData
		vmovaps ymm15, [r10] ;Load fftDataH
		
		vmulps ymm12, ymm1, ymm11
		vroundps ymm13, ymm12, 00b;{rn-sae} ;round to nearest
		
		vfmsub231ps ymm11, ymm2, ymm13
		vfmadd231ps ymm11, ymm3, ymm13
		
		vcvtps2dq ymm12, ymm12
		vpslld ymm12, ymm12, 23 ;mantissa length
		
		vmovaps ymm13, ymm4
		vfmadd213ps ymm13, ymm11, ymm5
		vfmadd213ps ymm13, ymm11, ymm6
		vfmadd213ps ymm13, ymm11, ymm7
		vfmadd213ps ymm13, ymm11, ymm8
		vfmadd213ps ymm13, ymm11, ymm0
		vfmadd213ps ymm13, ymm11, ymm0 ;extra polynomial precision
		
		vpaddd ymm11, ymm12, ymm13
		
		vaddps ymm11, ymm0, ymm11
		vdivps ymm11, ymm0, ymm11
		;sigmoid result is in ymm11
		
		vmaxps ymm11, ymm11, ymm10 ;Clamp
		
		vmulps ymm14, ymm14, ymm11
		vmulps ymm15, ymm15, ymm11
		
		vmovaps [rdx], ymm14 ;Store modified fftData
		vmovaps [r10], ymm15 ;Store modified fftDataH
		;vmovaps?
		
		add rdx, 0x20
		add r10, 0x20
		
		dec eax
	jnz .innerLoop
	
ret ;End of sigmoidClampMultiply

