; syscalls
(defun sysexit ((int code)) {
    (asm "mov $0x01, %eax")
    (asm "mov #code#, %ebx")
    (asm "mov (%ebx), %ebx")
    (asm "int $0x80")
    (asm "push %eax")
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
    ; TODO: fix me
    (resvstr string 12)
    (if (< n 10) {
        (setptr (+ (addr string) 0) (+ n 48))
        (setptr (+ (addr string) 1) 0)
        (return string)
    })
    (setptr (+ (addr string) 11) (+ (% n 10) 48))
    (defint length 10)
    (defint mod 0)
    (defint x (/ (abs n) 10))
    (while (> x 10) {
        (define mod (% x 10))
        (define x (/ x 10))
        (setptr (+ (addr string) length) (+ mod 48)) ; '0' = 48
        (define length (- length 1))
    })
    (if (< n 0) {
        (setptr (+ (addr string) length) 45) ; '-' = 48
    })
    (sysexit length)
    (return (+ (addr string) length))
})

(defun abs ((int n)) {
    (if (< n 0) {
        (return (- n))
    })
    (return n)
})

; print functions
(defun print ((str string)) {
    (defint length (strlen string))
    (return (syswrite 0 string length))
})

(defun println ((str string)) {
    (print string)
    (syswrite 0 "\n" 1)
    (return 0)
})

(println (itoa 10))
