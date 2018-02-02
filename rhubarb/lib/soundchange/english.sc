*
* Variables
*
* vowels, long, short
U=aeiou
V=aeiou������������@
L=�����������������@
S=�����������������@
A=a��
E=e��
I=i��
O=o��
&=eiou
* front
F=ei��y
* any letter
X=bcdfghjklmnpqrstvwxyz�+$�aeiou������������@
* consonants
C=bcdfghjklmnpqrstvwxyz�+$�
* dentals, liquids, nasals
D=td+
R=rl
M=mn�
T=tdns+
* stops, fricatives (voiced and voiceless)
P=ptk
B=bdg
�=fs$+
Z=vz#+
*
* Rules
*
* get rid of some digraphs
ch/�/_
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
ough/�/_t
augh/�/_t
ough/�/_
gh//_
* unpronounceable combinations
g//#_n
k//#_n
m//#_n
p//#_t
p//#_s
t//#_m
* medial y = i
y/�/#C_#
y/�/#CC_#
y/�/#CCC_#
ey/�/_
ay/�/_
oy/�y/_
y/i/C_C
y/i/C_#
y/i/C_e#
ie/�/CC_#
ie/�/#C_#
* sSl can simplify
t//s_lV#
* affrication of t + front vowel
ci/$/X_V
ti/$/X_V
tu/�u/X_V
tu/�u/X_RV
si/$/C_o
si/j/V_o
s/$/C_ur
s/j/V_ur
s/$/k_uV
s/$/k_uR
* intervocalic s
s/z/&_V
* al to ol (do this before respelling)
a/�/_ls
a/�/_lr
a/�/_ll#
a/�/_lm(V)#
a/�/C_lD
a/�/#_lD
al/�/X_k
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
�/�/_nd#
�/�/_ss#
�/�/_g#
�/�/_fC
�/�/_lD
�/�/w_$
�/�/w_(t)�
�/�/w_T
* soft gn
�g/�/_M#
�g/�/_MC
g//ei_n
* handle ous before removing -e
ou/@/_s#
ou/@/_sC
* remove silent -e
e//VC(C)(C)_#
* common suffixes that hide a silent e
�//XXX_m�nt#
�//XXX_n�ss#
�//XXX_li#
�//XXX_f�l#
* another common suffix
�/�/XXX_n�ss#
* shorten (1-char) weak penults after a long
* note: this error breaks almost as many words as it fixes...
L/S/LC(C)(C)_CV#
* double vowels
eau/�/_
ai/�/_
au/�/_
�w/�/_
ee/�/_
ea/�/_
ei/�/s_
ei/�/_
eo/�@/_
�w/�/_
eu/�/_
ie/�/_
V/@/i_
i/�/#C(C)_
i/�/_@
oa/�/_
oe/�/_#
oo/�/_k
oo/u/_
oul/�/_d#
ou/�w/_
oi/�y/_
ua/�@/_
ue/u/_
ui/u/_
�w/�/_#
* those pesky final syllabics
V/@/VC(V)_l#
�/@/VC(C)_n#
�/@/VC(C)_n#
�/@/VC(C)_n#
�/@/VC(C)_n#
* suffix simplifications
A/@/XXX_b@l#
�/y/Xl_@n#
�/y/Xn_@n#
* unpronounceable finals
b//m_#
n//m_#
* color the final vowels
a/@/_#
e/�/_#
i/�/_#
o/�/_#
* vowels before r  V=aeiou������������@
�w/�/_rX
�/�/_r
�/�/_r
�/�/w_rC
�/�/w_r#
�/�/_rr
�/�/_rIC
�/�/_rr
�/�/_rC
�/�/_r#
�/�/_r
�/@/_r
�/@/_r
�/@/_r
�/@/_r
* handle ng
ng/�/_�
ng/�/_B
ng/�/_P
ng/�/_#
n/�/_g
n/�/_k
�/�/_�
�/�/_�
* really a morphophonological rule, but it's cute
s/z/B_#
s/z/_m#
* double consonants
s//_s
s//_$
t//_t
t//_�
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
