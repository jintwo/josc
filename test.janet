(import /build/josc :as josc)

(defn write-struct-message [s]
  (josc/write-message (s :address) (s :format) (splice (s :data))))

# test serializtion-deserialization
(print "\n\nroundtrip1\n\n")
(def msg0 (write-struct-message {:address "/blob" :format "bif" :data @["blob" 10 13.4]}))
(def msg1 (josc/write-message "/blob" "bif" "blob" 10 13.4))
(def s0 (josc/parse-message msg0))
(def s1 (josc/parse-message msg1))
(pp s0)
(pp s1)

(print "\n\nroundtrip2\n\n")
(pp (josc/parse-message (write-struct-message {:address "/test" :format "isi" :data @[10 "str-str-str" 11]})))
