;; This buffer is for text that is not saved, and for Lisp evaluation.
;; To create a file, visit it with C-x C-f and enter text in its buffer.

(defvar *small* 1)
(defvar *big* 1000)




(defun guess-my-number ()
  (ash (+ *small* *big*) -1))

(defun smaller ()
  (setf *big* (1- (guess-my-number)))
  (message "%d" (guess-my-number)))


(defun bigger ()
  (setf *small* (1+ (guess-my-number)))
  (message "%d" (guess-my-number)))
  
(defun reset ()
  (setf *small* 1)
  (setf *big* 1000))



(guess-my-number)
(smaller)
(bigger)
(reset)
