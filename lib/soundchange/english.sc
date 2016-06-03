*
* Variables
*
* vowels, long, short
U=aeiou
V=aeiouäëïöüâêîôûùò@
L=äëïöüäëïöüäëïöüùò@
S=âêîôûâêîôûâêîôûùò@
A=aâä
E=eêë
I=iîï
O=oôö
&=eiou
* front
F=eiêîy
* any letter
X=bcdfghjklmnpqrstvwxyzç+$ñaeiouäëïöüâêîôûùò@
* consonants
C=bcdfghjklmnpqrstvwxyzç+$ñ
* dentals, liquids, nasals
D=td+
R=rl
M=mnñ
T=tdns+
* stops, fricatives (voiced and voiceless)
P=ptk
B=bdg
ß=fs$+
Z=vz#+
*
* Rules
*
* get rid of some digraphs
ch/ç/_
sh/$/_
ph/f/_
th/+/_
qu/kw/_
* and other spelling-level changes
w//_r
w//_ho
h//w_
h//#r_
h//x_
h//V_#
x/gz/#e_V
x/ks/_
'//_
* gh is particularly variable
gh/g/_V
V/L/C_gh
ough/ò/_t
augh/ò/_t
ough/ö/_
gh//_
* unpronounceable combinations
g//#_n
k//#_n
m//#_n
p//#_t
p//#_s
t//#_m
* medial y = i
y/ï/#C_#
y/ï/#CC_#
y/ï/#CCC_#
ey/ë/_
ay/ä/_
oy/öy/_
y/i/C_C
y/i/C_#
y/i/C_e#
ie/ï/CC_#
ie/ï/#C_#
* sSl can simplify
t//s_lV#
* affrication of t + front vowel
ci/$/X_V
ti/$/X_V
tu/çu/X_V
tu/çu/X_RV
si/$/C_o
si/j/V_o
s/$/C_ur
s/j/V_ur
s/$/k_uV
s/$/k_uR
* intervocalic s
s/z/&_V
* al to ol (do this before respelling)
a/ò/_ls
a/ò/_lr
a/ò/_ll#
a/ò/_lm(V)#
a/ò/C_lD
a/ò/#_lD
al/ò/X_k
* soft c and g
c/s/_F
c/k/_
ge/j/X_a
ge/j/X_o
g/j/_F
* init/final guF was there just to harden the g
gu/g/#_F
gu/g/_e#
* untangle reverse-written final liquids
re/@r/C_#
le/@l/C_#
* vowels are long medially
U/L/C_CV
U/L/#_CV
* and short before 2 consonants or a final one
U/S/C_CC
U/S/#_CC
U/S/C_C#
U/S/#_C#
* special but general rules
î/ï/_nd#
ô/ò/_ss#
ô/ò/_g#
ô/ò/_fC
ô/ö/_lD
â/ò/w_$
â/ò/w_(t)ç
â/ô/w_T
* soft gn
îg/ï/_M#
îg/ï/_MC
g//ei_n
* handle ous before removing -e
ou/@/_s#
ou/@/_sC
* remove silent -e
e//VC(C)(C)_#
* common suffixes that hide a silent e
ë//XXX_mênt#
ë//XXX_nêss#
ë//XXX_li#
ë//XXX_fûl#
* another common suffix
ï/ë/XXX_nêss#
* shorten (1-char) weak penults after a long
* note: this error breaks almost as many words as it fixes...
L/S/LC(C)(C)_CV#
* double vowels
eau/ö/_
ai/ä/_
au/ò/_
âw/ò/_
ee/ë/_
ea/ë/_
ei/ë/s_
ei/ä/_
eo/ë@/_
êw/ü/_
eu/ü/_
ie/ë/_
V/@/i_
i/ï/#C(C)_
i/ë/_@
oa/ö/_
oe/ö/_#
oo/ù/_k
oo/u/_
oul/ù/_d#
ou/ôw/_
oi/öy/_
ua/ü@/_
ue/u/_
ui/u/_
ôw/ö/_#
* those pesky final syllabics
V/@/VC(V)_l#
ê/@/VC(C)_n#
î/@/VC(C)_n#
â/@/VC(C)_n#
ô/@/VC(C)_n#
* suffix simplifications
A/@/XXX_b@l#
ë/y/Xl_@n#
ë/y/Xn_@n#
* unpronounceable finals
b//m_#
n//m_#
* color the final vowels
a/@/_#
e/ë/_#
i/ë/_#
o/ö/_#
* vowels before r  V=aeiouäëïöüâêîôûùò@
ôw/ö/_rX
ô/ö/_r
ò/ö/_r
â/ö/w_rC
â/ö/w_r#
ê/ä/_rr
ë/ä/_rIC
â/ä/_rr
â/ô/_rC
â/ô/_r#
â/ä/_r
ê/@/_r
î/@/_r
û/@/_r
ù/@/_r
* handle ng
ng/ñ/_ß
ng/ñ/_B
ng/ñ/_P
ng/ñ/_#
n/ñ/_g
n/ñ/_k
ô/ò/_ñ
â/ä/_ñ
* really a morphophonological rule, but it's cute
s/z/B_#
s/z/_m#
* double consonants
s//_s
s//_$
t//_t
t//_ç
p//_p
k//_k
b//_b
d//_d
d//_j
g//_g
n//_n
m//_m
r//_r
l//_l
f//_f
z//_z
