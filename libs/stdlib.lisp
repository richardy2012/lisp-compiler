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
(defun strat ((str string) (int n)) {
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
    (defint chr (strat string 0))
    (while (!= chr 0) {
        (define chr (strat string n))
        (define n (+ n 1))
    })
    (return n)
})

; number functions
(defun itoa ((int n)) {
    (defptr string 0)
    (defint mod 0)
    (while (> n 0) {
        (define mod (% n 10))
        ; TODO
        (define n (/ n 10))
    })
    (return string)
})

(defun abs ((int n)) {
    (if (< n 0) {
        (return (- n))
    } {
        (return n)
    })
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
