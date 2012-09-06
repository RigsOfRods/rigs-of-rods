#!/bin/sh

# update trunk first
BASE=/var/svn-co/trunk
WEBDIR=/var/www/docs.rigsofrods.com/htdocs
INDEX=${WEBDIR}/index.html
INDEX2=${WEBDIR}/index2.html
ZIPFN=ror-doc.zip
ZIP=${WEBDIR}/${ZIPFN}
# we need to cd, since doxygen paths work locally
cd $BASE

/usr/bin/svn up $BASE #2>&1 >>/dev/null

echo "the documentation is currently being updated, please check back in some minutes" > $INDEX

echo "<html><body><h2>Rigs of Rods Documentation overview</h2><ul>" > $INDEX2
chown lighttpd:lighttpd $INDEX $INDEX2

CONFIGS=$(ls $BASE/doc/*.conf | grep ".linux.")
for CONFIG in $CONFIGS
do
 CONFIGFILE=$(basename $CONFIG)
 echo $CONFIGFILE
 NAME=$(echo $CONFIG | awk -F. '{ print $2 }')
 PROJECT=$(cat $CONFIG | grep "PROJECT_NAME" | grep -v "#" | awk -F= '{ print $2 }' | sed "s/\"//g" | sed -e 's/^[ \t]*//')
 OUTDIR=$(cat $CONFIG | grep "OUTPUT_DIRECTORY" | grep -v "#" | awk -F= '{ print $2 }' | sed -e 's/^[ \t]*//')
 if [[ "$NAME" == "" ]]
 then
  # do not allow to delete the root dir ...
  continue
 fi

 # delete old contents
 rm -rf $WEBDIR/$NAME/
 # generate new content
 mkdir -p $BASE/$OUTDIR
 mkdir -p $WEBDIR/$NAME/
 #echo "/usr/bin/doxygen $CONFIG"
 /usr/bin/doxygen $CONFIG  > $WEBDIR/$NAME/doxygen_stdout.log 2>$WEBDIR/$NAME/doxygen_stderr.log
 
 # copy new contents
 cp -rp $BASE/$OUTDIR/html/* $WEBDIR/$NAME/

 # add to index
 echo "
 <li>
  <a style='font-size:1.4em' href='/$NAME/'>$PROJECT</a><br/>
  <span style='font-size:0.7em'>(doxygen build log: <a href=\"$NAME/doxygen_stdout.log\">stdout</a>, <a href=\"$NAME/doxygen_stderr.log\">stderr</a>)</span><br/>&nbsp;
 </li>" >> $INDEX2
done
echo "</ul>" >> $INDEX2



# finish the index html site
LASTCHANGE=$(svn info ${BASE} | grep Last)
REV=$(svn info ${BASE} | grep Revision)
DATET=$(date)

echo "
download zipped version of this documentation: <a href='${ZIPFN}'>${ZIPFN}</a><br/>
<pre>last update: $DATET
svn trunk ${REV}
${LASTCHANGE}
</pre>

(this updates automatically daily)</body></html>" >> $INDEX2

mv $INDEX2 $INDEX

# zip the archive
cd ${WEBDIR}
/usr/bin/zip -rqu ${ZIP} .


# fix permissions
chown -R lighttpd:lighttpd $WEBDIR/*

