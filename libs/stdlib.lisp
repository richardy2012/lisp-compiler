; syscalls
(defun sysexit ((int code)) {
    (asm "mov $0x01, %eax")
    (asm "mov #code#, %ebx")
    (asm "mov (%ebx), %ebx")
    (asm "int $0x80")
    (return 0)
})

(defun syswrite ((int fd) (str buf) (int len)) {
    (asm "mov $0x04, %eax")
    (asm "mov #fd#, %ebx")
    (asm "mov (%ebx), %ebx")
    (asm "mov #buf#, %ecx")
    (asm "mov (%ecx), %ecx")
    (asm "mov #len#, %edx")
    (asm "mov (%edx), %edx")
    (asm "int $0x80")
    (return 0)
})

; string functions
(defun char ((str string) (int n)) {
    (defptr chr (+ string n))
    (asm "mov #chr#, %eax")
    (asm "mov #chr#, %ebx")
    (asm "mov (%eax), %eax")
    (asm "mov (%eax), %al")
    (asm "movzx %al, %ecx")
    (asm "mov %ecx, (%ebx)")
    (return chr)
})

(defun strlen ((str string)) {
    (defint n 0)
    (defint chr (char string 0))
    (while (!= chr 0) {
        (define chr (char string n))
        (define n (+ n 1))
    })
    (return n)
})

; number functions
(defun itoa ((int n)) {
    (resvstr string 11)
    (defint n (abs n))
    (defint mod 0)
    (defint length 0)
    (while (> n 0) {
        (define mod (% n 10))
        (define n (/ n 10))
        (define length (+ length 1))
    })
    (return string)
})

(defun abs ((int n)) {
    ; TODO: convert this to lisp
    (asm "mov #n#, %eax")
    (asm "mov (%eax), %eax")
    (asm "movl %eax, %ebx")
    (asm "negl %eax")
    (asm "cmovl %ebx, %eax")
    (asm "pushl %eax")
})

; print functions
(defun print ((str string)) {
    (defint length (strlen string))
    (return (syswrite 0 string length))
})

(defun println ((str string)) {
    (defint length (strlen string))
    (return (syswrite 0 string length))
})
