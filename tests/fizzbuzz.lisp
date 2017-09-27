(define n 1)
(while (< n 101) {
    (if (and (= (% n 5) 0) (= (% n 3) 0)) {
        (println "FizzBuzz")
    } (if (= (% n 3) 0) {
        (println "Fizz")
    } (if (= (% n 5) 0) {
        (println "Buzz")
    }  (println (itoa n)))))
    (define n (+ n 1))
})
