(define x 4)
(define y 5)
(while x {
    (while y {
        (print "y = ")
        (println (itoa y))
        (define y (- y 1))
    })
    (print "x = ")
    (println (itoa x))
    (define x (- x 1))
})
