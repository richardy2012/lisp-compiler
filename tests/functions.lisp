(defun A () 12)
(println (itoa (A 12)))
(defun B ((int a) (int b)) (+ a b 15))
(println (itoa (B 30 40)))
(defun C ((int a) (int b) (int c)) b)
(println (itoa (C 30 60 70)))
(defun D ((str a)) (println a))
(D "Hello World")
