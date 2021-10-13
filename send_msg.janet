# Send OSC msg to any OSC server
# `Protokol` by Hexler nice tool for debugging/testing OSC
(import /build/josc :as josc)

## check serialization
# (def conn (net/connect "127.0.0.1" "8008" :datagram))
# (:write conn (josc/write-message "/blob_i" "biifbf" "blob" 10 11 12.2 "bolb" 13.3))
# (:write conn (josc/write-message "/string" "sf" "string" 13.3))
# (:write conn (josc/write-message "/int" "i" 1))
# (:write conn (josc/write-message "/float" "f" 1.0))

## sending messages to KushView Element
# (def conn (net/connect "127.0.0.1" "8008" :datagram))
# (def channel (ev/chan 5))
#
# (defn make-note-on [note &opt channel velocity]
#   (josc/write-message "/midi/noteOn" "iif" (or channel 0) note (or velocity 1.0)))
#
# (defn make-note-off [note &opt channel velocity]
#   (josc/write-message "/midi/noteOff" "iif" (or channel 0) note (or velocity 1.0)))
#
# (ev/spawn
#  (forever
#   (ev/give channel (make-note-on 62))
#   (ev/sleep 1.0)
#   (ev/give channel (make-note-off 62))))
#
# (forever
#  (:write conn (ev/take channel))
#  (ev/sleep 0.5))
