#!/bin/bash
echo "Comienza venta recuperada" > ~/recover_items
echo "0" >> ~/recover_items
echo "0" >> ~/recover_items
cat ~/last_items >> ~/recover_items
echo "Termina venta recuperada" >> ~/recover_items
echo "0" >> ~/recover_items
echo "0" >> ~/recover_items
echo "" >> ~/recover_items
echo -n "T" >> ~/recover_items
echo -n "C" >> ~/recover_items
echo -n "tttt" >> ~/recover_items
echo "" >> ~/recover_items
echo "" >> ~/recover_items
#/usr/local/bin/venta < ~/recover_items


