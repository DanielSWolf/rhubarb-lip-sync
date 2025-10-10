#!/bin/bash

set -x

rsync -aP html index.html manual.html build.html style.css \
      $USER@web.sourceforge.net:/home/project-web/tclap/htdocs/v1.4

rsync -aP html index.html manual.html build.html style.css \
      $USER@web.sourceforge.net:/home/project-web/tclap/htdocs/
