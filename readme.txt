= Tool for fitting Bayesian Knowledge Tracing models =

This tool is developed to fit Bayesian Knowledge Teacing models efficiently on
large datasets. It is a command line utility written in C/C++. Most of the
development was done while the author was a posdoc at Human Computer Interaction
Institute, Carnegie Mellon Univerisy. This tool can be used, modified,
re-distributed freely. We only ask you to acknowledge it.

= Bayesian Knowledge Tracing =

Paper

= Compilation =

If you are on a Linux or Mac OS X system, simply use 'make all' command. If you
are on Windows, you will need to install cygwin and have g++/gcc compiler
available.

= Data =

Input file data format is quite simple. Four tab separated columns: observation,
student problem/problem step, skill(s). Observation is a 1 started integer. We
advise to use 1 for 'correct' and 2 for 'incorrect'. Student is a string label, so
as problem or problem step, whatever granularity you prefer. Skill is a string
label. Multiple skill labels should be delimited by a character of your choice
(do not use tab). An example of few lines of input is below.

2   student_001 unit1-secion1-problem5-step1  addition~multiplication
1   student_001 unit1-secion1-problem5-step2  multiplication
1   student_001 unit1-secion1-problem5-step3  addition

A sample data file is supplied. This data is generated using the following 
parameters: pLo, pT, pS, pG

To give this tool a proper test you might want to try it on a KDD Cup 2010
dataset donated by Carnegie Learning Inc. This datasets consists of training and
challenge sets that consist of multitude of data columns. For the sake of
testing the tool, download the challenge set Algebra I that has about 9 million
transactions of over 3300 students. The training file should be trimmed to the 
tool's format. See shell commands below that do that.

sh> gawk '-F\t' 'BEGIN{OFS=""} {print ".","\t",$2,"\t",$3,"__",$4,"\t",tolower($20)}' algebra_2008_2009_train.txt > tmp1.txt
sh> sed 1d tmp1.txt
sh> rm tmp1.txt
sh> awk '-F\t' 'BEGIN{OFS=""} {print $1,"\t",$2,"\t",$3,"\t",((length($4)==0)?".":$4)}' tmp2.txt > a89_kts_train.txt
sh> rm tmp2.txt

To fit a BKT model of this dataset using gradient descent method as well as to 
computerun fir metrics and the prediction enter the following command.

sh> trainhmm -s 1.2 -m 1 -p 1 a89_kts_train.txt model.txt predict.txt

= References =

[1] Corbett Anderson

= Contact Us =

If you have any questions, comments, propositions, feel free to contact me at
myudelson@gmail.com

Have fun fitting BKT models,
Michael (Mikhail) Yudelson
