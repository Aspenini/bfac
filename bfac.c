// bfac.c — BFAC: Brainfuck Advanced Compiler with bundled GoAsm/GoLink
// Build: build.bat
// Usage: bfac input.bf output.exe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char* argv[]) {
    int loop_stk[4096];
    int loop_stk_ptr = 0;
    int loop_count = 0;

    if(argc != 3) {
        printf("BFAC - Brainfuck Advanced Compiler v1.0\n");
        printf("Usage: bfac input.bf output.exe\n");
        printf("\nGenerates native Windows PE executables with zero dependencies!\n");
        return -1;
    }

    printf("Compiling: %s -> %s\n", argv[1], argv[2]);

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        printf("Error: Cannot open input file %s\n", argv[1]);
        return -1;
    }

    // Create temporary assembly file (use local directory for debugging)
    char temp_asm[MAX_PATH];
    strcpy(temp_asm, "debug_temp.asm");
    
    FILE* output = fopen(temp_asm, "w");
    if (!output) {
        printf("Error: Cannot create temporary assembly file\n");
        return -1;
    }

    // Generate GoAsm-compatible Windows x64 assembly
    fprintf(output, "; BFAC generated assembly\n");
    fprintf(output, "\n");
    fprintf(output, ".data\n");
    fprintf(output, "tape db 65536 dup (?)\n");
    fprintf(output, "bytes_written dd ?\n");
    fprintf(output, "bytes_read dd ?\n");
    fprintf(output, "stdout_handle dq ?\n");
    fprintf(output, "stdin_handle dq ?\n");
    fprintf(output, "\n");
    fprintf(output, ".code\n");
    fprintf(output, "\n");
    fprintf(output, "start:\n");
    fprintf(output, "    sub rsp, 40        ; Shadow space + alignment\n");
    fprintf(output, "\n");
    
    // Get stdout handle
    fprintf(output, "    mov rcx, -11       ; STD_OUTPUT_HANDLE\n");
    fprintf(output, "    call GetStdHandle\n");
    fprintf(output, "    mov [stdout_handle], rax\n");
    fprintf(output, "\n");
    
    // Get stdin handle  
    fprintf(output, "    mov rcx, -10       ; STD_INPUT_HANDLE\n");
    fprintf(output, "    call GetStdHandle\n");
    fprintf(output, "    mov [stdin_handle], rax\n");
    fprintf(output, "\n");
    
    // Initialize tape pointer
    fprintf(output, "    lea r15, [tape]    ; r15 = tape pointer\n");
    fprintf(output, "\n");

    // Process Brainfuck code
    char c;
    while ((c = fgetc(input)) != EOF) {
        switch(c) {
            case '>':
                fprintf(output, "    inc r15\n");
                break;

            case '<':
                fprintf(output, "    dec r15\n");
                break;

            case '+':
                fprintf(output, "    inc B[r15]\n");
                break;

            case '-':
                fprintf(output, "    dec B[r15]\n");
                break;

            case '.':
                // WriteFile(stdout_handle, r15, 1, &bytes_written, NULL)
                fprintf(output, "    mov rcx, [stdout_handle]\n");
                fprintf(output, "    mov rdx, r15\n");
                fprintf(output, "    mov r8, 1\n");
                fprintf(output, "    lea r9, [bytes_written]\n");
                fprintf(output, "    mov Q[rsp+32], 0  ; lpOverlapped = NULL\n");
                fprintf(output, "    call WriteFile\n");
                break;

            case ',':
                // ReadFile(stdin_handle, r15, 1, &bytes_read, NULL)
                fprintf(output, "    mov rcx, [stdin_handle]\n");
                fprintf(output, "    mov rdx, r15\n");
                fprintf(output, "    mov r8, 1\n");
                fprintf(output, "    lea r9, [bytes_read]\n");
                fprintf(output, "    mov Q[rsp+32], 0  ; lpOverlapped = NULL\n");
                fprintf(output, "    call ReadFile\n");
                break;

            case '[':
                fprintf(output, "loop_start_%d:\n", loop_count);
                fprintf(output, "    cmp B[r15], 0\n");
                fprintf(output, "    je loop_end_%d\n", loop_count);
                loop_stk[loop_stk_ptr++] = loop_count++;
                break;

            case ']':
                if (loop_stk_ptr == 0) {
                    printf("Error: Unmatched ']'\n");
                    return -1;
                }
                int loop_id = loop_stk[--loop_stk_ptr];
                fprintf(output, "    cmp B[r15], 0\n");
                fprintf(output, "    jne loop_start_%d\n", loop_id);
                fprintf(output, "loop_end_%d:\n", loop_id);
                break;
        }
    }

    if (loop_stk_ptr != 0) {
        printf("Error: Unmatched '['\n");
        return -1;
    }

    // Exit program
    fprintf(output, "\n");
    fprintf(output, "    mov rcx, 0         ; Exit code 0\n");
    fprintf(output, "    call ExitProcess\n");

    fclose(input);
    fclose(output);
    
    printf("Assembly file created: %s\n", temp_asm);
    
    // Now assemble and link using bundled tools
    char goasm_path[MAX_PATH], golink_path[MAX_PATH];
    char exe_dir[MAX_PATH];
    
    // Get directory where bf2pe.exe is located
    GetModuleFileName(NULL, exe_dir, MAX_PATH);
    char *last_slash = strrchr(exe_dir, '\\');
    if (last_slash) *last_slash = 0;
    
    // Build paths to tools (in ./tools relative to exe)
    snprintf(goasm_path, sizeof(goasm_path), "%s\\tools\\GoAsm.exe", exe_dir);
    snprintf(golink_path, sizeof(golink_path), "%s\\tools\\GoLink.exe", exe_dir);
    
    // Check if tools exist
    if (GetFileAttributes(goasm_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Error: GoAsm.exe not found at %s\n", goasm_path);
        printf("Make sure the tools folder contains GoAsm.exe and GoLink.exe\n");
        DeleteFile(temp_asm);
        return -1;
    }
    
    printf("Assembling with GoAsm...\n");
    
    // Create temp obj file name in tools directory
    char temp_obj[MAX_PATH];
    snprintf(temp_obj, sizeof(temp_obj), "%s\\tools\\temp.obj", exe_dir);
    
    // Copy assembly file to tools directory  
    char tools_asm[MAX_PATH];
    snprintf(tools_asm, sizeof(tools_asm), "%s\\tools\\temp.asm", exe_dir);
    CopyFile(temp_asm, tools_asm, FALSE);
    
    // Assemble with GoAsm - use cmd /c to change directory
    char cmd[1024];
    char tools_dir[MAX_PATH];
    snprintf(tools_dir, sizeof(tools_dir), "%s\\tools", exe_dir);
    
    snprintf(cmd, sizeof(cmd), "cmd /c \"cd /d \"%s\" && GoAsm.exe /x64 temp.asm\"", tools_dir);
    printf("Running: %s\n", cmd);
    int result = system(cmd);
    
    if (result != 0) {
        printf("Error: GoAsm failed\n");
        printf("Assembly file left at: %s for debugging\n", temp_asm);
        // DeleteFile(temp_asm);  // Don't delete for debugging
        return -1;
    }
    
    printf("Linking with GoLink...\n");
    
    // Link with GoLink - use cmd /c to stay in tools directory
    snprintf(cmd, sizeof(cmd), "cmd /c \"cd /d \"%s\" && GoLink.exe /console /entry:start /fo temp.exe temp.obj kernel32.dll\"", tools_dir);
    printf("Running: %s\n", cmd);
    result = system(cmd);
    
    if (result != 0) {
        printf("Error: GoLink failed\n");
        DeleteFile(temp_asm);
        DeleteFile(temp_obj);
        return -1;
    }
    
    // Move the output executable to the desired location
    char tools_exe[MAX_PATH];
    snprintf(tools_exe, sizeof(tools_exe), "%s\\tools\\temp.exe", exe_dir);
    
    if (!MoveFile(tools_exe, argv[2])) {
        printf("Error: Could not move output file to %s\n", argv[2]);
        DeleteFile(temp_asm);
        DeleteFile(temp_obj);
        DeleteFile(tools_exe);
        return -1;
    }
    
    // Cleanup
    DeleteFile(temp_asm);
    DeleteFile(temp_obj);
    
    printf("Success! Created: %s\n", argv[2]);
    printf("Generated native Windows PE executable with zero dependencies!\n");
    
    return 0;
}
