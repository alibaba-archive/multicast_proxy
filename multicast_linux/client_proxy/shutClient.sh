#!/bin/bash
id=`ps|grep multic_admin|awk '{print $1}'`
kill $id
