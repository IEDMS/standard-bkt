= Tool for fitting Bayesian Knowledge Tracing models =

This tool is developed to fit Bayesian Knowledge Teacing models efficiently on
large datasets. It is a command line utility written in C/C++. Most of the
development was done while the author was a posdoc at Human-Computer Interaction
Institute, Carnegie Mellon Univerisy. These sources are published under BSD-new
(3-clause BSD) licence.

= Bayesian Knowledge Tracing =

BKT is a user modeling approach in wide use in the area of Intelligent Tutoring
Systems. The goal of BKT is to infer whether student has mastered a skill or not
from a pattern of successful or unsuccessful attempts to apply the skill. It is
a special case of Hidden Markov Model (HMM). In BKT there are two types of
nodes: binary state nodes capture skill mastery (mastered or not) and binary
observation nodes (correct or incorrect application of a skill).

For each skill BKT has four parameters.

1) pInit or pLo - is a probability the skill was known a priori,
2) pLearn or pT - is a probability the skill with transition into "mastered"
    state upon practice attempt,
3) pSlip or PS - is a probability that a known skill is applied incorrectly, and
4) pGuess or pG - is a probability that unknown skill will be applied correctly.

There is no forgetting in BKT and it's pForget or pF is set to zero. In
addition, there is a pKnown or pL parameters, which is a running estimate of the
skill mastery.

For more details on BKT refer to [1].



= Compilation =

If you are on a Linux or Mac OS X system, simply use 'make all' command. If you
are on Windows, you might need to install cygwin and have g++/gcc compiler
available.

= Data =

Input file data format is quite simple. Four tab separated columns: observation,
student, problem/problem step, skill(s). Observation is a 1-started integer. For
the two-state BKT, we advise to use 1 for 'correct' and 2 for 'incorrect'.
Student is a string label, so as problem or problem step, whatever granularity
you prefer. Skill is a string label. Multiple skill labels should be delimited
by a character of your choice (do not use tab). An example of few lines of input
is below.

2   student_001 unit1-secion1-problem5-step1  addition~multiplication
1   student_001 unit1-secion1-problem5-step2  multiplication
1   student_001 unit1-secion1-problem5-step3  addition

= Training BKT models =

= Using models for prediction =

Smal sample data file <toy_data.txt> is generated using the following BKT 
parameters: pLo=0.4, pT=0.35, pS=0.25, pG=0.12 .

To fit a BKT model of this data using an EM algorithm run the following command

./trainhmm -s 1.1 -m 1 -p 1 toy_data.txt model.txt predict.txt

The model will have 90% accuracy and root mean squared error (RMSE) = 0.3227and
the recovered BKT parameters would be: pLo=0.50, pT=0.17, pS=0.00, pG=0.00 .

If we fit BKT model using Conjugate Gradient method using '-s 1.2' argument, the
recovered parameters would be: pLo=0.00, pT=0.18, pS=0.08, pG=0.03, the accuracy
would remain at 90% while RMSE = 0.2982.

To give this tool a proper test you might want to try it on a KDD Cup 2010
dataset donated to the Pittsburgh Science of Learning Center by Carnegie
Learning Inc. The dataset can be downloaded (after a quick registration) from
http://pslcdatashop.web.cmu.edu/KDDCup/. This datasets consists of training and
challenge sets. For the sake of testing the tool, download the challenge 
Algebra I set that has about 9 million transactions of over 3300 students. The
training file should be trimmed to the tool's format. See shell commands below
that do that.

sh> gawk '-F\t' 'BEGIN{OFS=""} {print ".","\t",$2,"\t",$3,"__",$4,"\t",tolower($20)}' algebra_2008_2009_train.txt > tmp1.txt
sh> sed 1d tmp1.txt
sh> rm tmp1.txt
sh> awk '-F\t' 'BEGIN{OFS=""} {print $1,"\t",$2,"\t",$3,"\t",((length($4)==0)?".":$4)}' tmp2.txt > a89_kts_train.txt
sh> rm tmp2.txt

To fit a BKT model of this dataset using gradient descent method as well as to 
compute fit metrics and the prediction run the following command:

sh> trainhmm -s 1.2 -m 1 -p 1 a89_kts_train.txt model.txt predict.txt

Depending on your hardware, the model should be fit in approximately 2 minutes.

= References =

[1] Corbett, A. T. and Anderson, J. R.: Knowledge tracing: Modeling the
    acquisition of procedural knowledge. User Modeling and User-Adapted
    Interaction, 4(4), 253-278. (1995)
[2] Levinson, S. E., Rabiner, L. R., and Sondhi, M. M.: An Introduction to the
    Application of the Theory of Probabilistic Functions of a Markov Process to
    Automatic Speech Recognition. Bell System Technical Journal, 62(4):
    1035-1074. (1983)
[3] 

= Contact Us =

If you have any questions, comments, propositions, feel free to contact me at
myudelson@gmail.com

Have fun fitting BKT models,
Michael (Mikhail) Yudelson
