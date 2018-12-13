Files related to translations:
- ror.pot: Template in English, serves as base for all translations, extracted from source code
- \<lang.po\>: translated version of ror.pot in specific language
- \<lang.mo\>: translation for \<lang\> in binary form (compiled from \<lang.po\>), this is what RoR uses at runtime

Basically these steps are required to get up to date translations:
- dev only: Extract ror.pot template from code, upload to Transifex (see below)
- wait for community to translate
- download \<lang.po\> for desired translations
- compile \<lang.mo\> for these translations (see below)
- put \<lang\>.mo in ```bin/languages/<lang>/LC_MESSAGES/ ```

Additional information: http://www.rigsofrods.org/wiki/pages/Language_Translation#How_it_works

### Update source file on Transifex
The source file (ror.pot) on Transifex serves as the base for all translations and
should be updated whenever new strings which need translation have been added to the source.

```
# extract new translations template
extract_pot.sh
# now update the source file on Transifex:
# RigsOfRods -> more project options -> resource -> ror.pot -> update
```

### Compile *.mo files from Transifex
A *.mo file is a translation for a given language in binary form.  
Rigs of Rods needs *.mo files for the translations to appear ingame.

```
# setup local Transifex repo, pull all translations
setup_transifex.sh
# copy translations from local Transifex repo to languages folder
from_transifex.sh
# compiles *.mo files for all languages where translations are available
compile_mo.sh
# The languages folder can now be put in the bin folder of RoR (same level as the RoR executable).
```




### Advanced
Additional scripts are provided for specific use cases such as 
updating the translations files manually. 
Transifex does this automatically so they should rarely be needed.

```
# extract new translations template
extract_pot.sh
# setup local Transifex repo, pull all translations
setup_transifex.sh
# copy translations from local Transifex repo to languages folder
from_transifex.sh
# merge old translations with new template
merge_old_translations.sh
# push updates to Transifex (requires changes inside script)
to_transifex.sh
<wait for community to translate>
# pull updated translations from Transifex
setup_transifex.sh
# copy translations from local Transifex repo to languages folder
from transifex.sh
# compile *.mo files
compile_mo.sh
# The languages folder can now be put in the bin folder of RoR (same level as the RoR executable).
```

