# remove shm-object-file on rome1
echo "Removing shm-obj-file on Rome1..."
rm /dev/shm/TEST_QUEUE_0

# remove shm-object-file on rome2
echo "Removing shm-obj-file on Rome2..."
ssh rome2 rm /dev/shm/TEST_QUEUE_1
