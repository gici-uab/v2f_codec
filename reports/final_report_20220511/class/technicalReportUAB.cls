% GICI Technical Report Template class
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% These commands control the template contents, 
% and are to be renewed in the client tex file

\newcommand{\externalOrganization}{External Inc.}
\newcommand{\externalProjectReference}{Ext/Proj/Ref}
\newcommand{\internalProjectReference}{UAB/DEIC/GICI-YYYYMMDD}
\newcommand{\reportReferenceAffix}{D0} % e.g., R0 for the first report
\newcommand{\reportReference}{\internalProjectReference-\reportReferenceAffix}
\newcommand{\documentTitle}{Long document title\\with as many lines\\\textbf{and format} as needed}
\newcommand{\pdfTitle}{PDF file title (no newline nor format)}
\newcommand{\pdfAuthor}{UAB - GICI}

\newcommand{\documentAbstract}{This document examines ... renew \textbackslash{}documentAbstract to write here }
\newcommand{\documentSignatures}{ renew \textbackslash{}documentSignatures to write here  }
\newcommand{\documentTypeName}{renew \textbackslash{}documentTypeName to write here}
\newcommand{\documentVersion}{renew \textbackslash{}documentVersion to write here}

\newcommand{\documentSubtitle}{renew \textbackslash{}documentSubtitle to write here}
\newcommand{\documentDate}{renew \textbackslash{}documentDate to write here}
\newcommand{\documentReference}{renew \textbackslash{}documentReference to write here}
\newcommand{\acompanyamentPrimerNivell}{\textbf{\documentTitle}\\\documentSubtitle}

\newcommand{\leftHeaderContents}{%
\textbf{\documentTitle}\\\
}

\newcommand{\rightHeaderContents}{%
Date: \documentDate \\
\documentReference\\
Document version: v\documentVersion
}

\newcommand{\leftFooterContents}{%
\documentReference\ v\documentVersion
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% v1: Ian.Blanes@uab.cat
% v2: Miguel.Hernandez@uab.cat

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{class/technicalReportUAB}[2019/9/21 Technical report class for UAB documents v2]
\LoadClass{article}


\usepackage{etex}

\usepackage[english]{babel}
\usepackage{makeidx}
\makeindex

\usepackage[scaled=.92]{helvet}
\usepackage[absolute,overlay]{textpos} % 
\usepackage[usenames,svgnames,table]{xcolor} % colors


\usepackage{algorithm}
\usepackage{algorithmicx}
\usepackage{algcompatible}
\usepackage{longtable}

\usepackage[printonlyused]{acronym} % Acronyms with its own section

\usepackage[normalem]{ulem} % Underlines that allow line breaks
\usepackage{pifont} % arrows / hands / pens / scisors
\usepackage{amsmath} % Extended math notation
\usepackage{amsfonts} % Extra math fonts
\usepackage{amssymb}
\usepackage{array} % For ragged text on tables
\usepackage{graphicx} % Include figures

\usepackage{xifthen}

% \usepackage[colorlinks=false,pdfborder={0 0 0},plainpages=false,pdftitle={\pdfTitle},pdfauthor={\pdfauthor}]{hyperref}
\usepackage[colorlinks=false,pdfborder={0 0 0},plainpages=false,pdftitle={\pdfTitle}]{hyperref}

\usepackage[english]{babel} % Hypenation
\usepackage{hyphenat} % \nohyphens{text...}
\usepackage{setspace}
\usepackage{url}
%\usepackage{multibib}
%\newcites{A}{Appendix A\\[20pt] List of all publications produced}
%\newcites{other}{List of all publications produced}
\usepackage{framed}
\usepackage{fancyhdr}
\usepackage{lastpage} % \pageref{LastPage}
\usepackage[us,24hr]{datetime} % \currenttime

% Unused but useful packages
\usepackage{rotating} % rotate tables
\usepackage{dcolumn} % Dot alignment on tables
%\usepackage{tabularx} % Vertical alignment for tables
%\usepackage{endnotes} % endnotes rather than footnotes
%\usepackage{booktabs}
%\usepackage{eurosym} % € symbol cal?
\usepackage{pdfpages} % insert external pdfs as full pages
\usepackage{setspace}
%\usepackage{amsmath} % Extended math notation
%\usepackage{amsfonts} % Extra math fonts
\usepackage{amssymb} % \ggg >>>
%\usepackage[final]{listings} % Code listings
%\usepackage[symbol]{footmisc} % symbols at footnotes
\usepackage{multirow} % Tables with cells that span multiple rows
\usepackage[labelfont=bf,textfont=bf]{caption} % Captions in figures
%\usepackage{xcolor}
\usepackage{tabularx}
\usepackage{tabulary}
\usepackage{colortbl}
%\usepackage{cite}
%\usepackage[]{eufrak}
%\usepackage{epsfig}
%\usepackage[]{setspace}
\usepackage{balance}
\usepackage{placeins} % \FloatBarrier (no floats pass this barrier)
\usepackage{multicol} % \begin{multicols}{2}
\usepackage{lipsum} % \lipsum[1-2]

\usepackage{fancyvrb}
\usepackage{enumitem} % \begin{enumerate}[label={[\arabic*]}]
\usepackage{parskip}
%\setlength{\parskip}{\largeskipamount} 
\usepackage{needspace} % to prevent page breaks in notes

%% UFT8 input
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}

\textblockorigin{0pt}{0pt}
%\textblockcolor{red}

\usepackage[a4paper,left=2cm,bottom=17mm,right=2cm,top=30mm,headheight=10mm,headsep=10mm,
footskip=17mm]{geometry}

\usepackage{fancyhdr}

\newcommand{\mypagestyle}[0]{
\fancyhead[L]{}\fancyhead[R]{}%
\fancyhead[C]{\hspace{-10mm}\begin{minipage}[t]{180mm}
\raisebox{0.1cm}{\parbox[b][25mm][t]{25mm}{\includegraphics[width=25mm]{class/logoUAB.pdf}}}\hspace*{10mm}%
\parbox[b][25mm][t]{80mm}{\noindent\fontsize{8}{10}\fontfamily{phv}\selectfont%
%
\leftHeaderContents
%\\
%
}\hspace*{15mm}\parbox[b][25mm][t]{50mm}{\noindent\fontsize{7}{9}\fontfamily{phv}\selectfont%
%
\raggedleft
\rightHeaderContents
%
}\end{minipage}\hspace{-10mm}%
\vspace{-13mm}\vspace{-10pt}%
}

\fancyfoot[C]{
\hspace{-10mm}%
\begin{minipage}[t]{180mm}
\parbox[b][10mm][t]{120mm}{\noindent\fontsize{8}{10}\fontfamily{phv}\selectfont%
%
\leftFooterContents
%
}\parbox[b][10mm][t]{60mm}{\raggedleft\noindent%
\fontsize{7}{9}\fontfamily{phv}\selectfont\thepage/\pageref{LastPage}}%
\end{minipage}\hspace{-10mm}%
}

\renewcommand{\headrulewidth}{0pt}
\renewcommand{\footrulewidth}{0pt}
}

\pagestyle{fancy}


\newcommand{\coverpage}{
%% Cover
\begin{titlepage}
\begin{textblock*}{50mm}(15mm,259.95mm)%
\noindent\includegraphics[width=50mm]{class/logoUABblack.pdf}%
\end{textblock*}
\begin{textblock*}{50mm}(100mm,259.95mm)%
\noindent{\includegraphics[width=50mm]{class/logodeic.pdf}}%
\end{textblock*}
\begin{textblock*}{50mm}(147mm,259.95mm)%
\noindent{\includegraphics[width=50mm]{class/logogici.pdf}}%
\end{textblock*}
\vspace*{1cm}
\begin{center}
\noindent\fontsize{22}{28}\fontfamily{phv}\selectfont%
\textbf{\documentTypeName}\\
\noindent\fontsize{32}{38}\fontfamily{phv}\selectfont%
\documentTitle\\[1cm]
\noindent\fontsize{22}{28}\fontfamily{phv}\selectfont%
\documentSubtitle
\end{center}
\vspace{30mm}

% \begin{minipage}{0.43\linewidth}
% \noindent
% % \fontsize{12}{14}\fontfamily{phv}\selectfont%
% \fontfamily{phv}\selectfont%

\begin{minipage}{\linewidth}
%     \centering
    \begin{minipage}{0.65\linewidth}
        \fontfamily{phv}\selectfont
        {\Large Abstract\normalsize} \\[5pt] 
        \documentAbstract
    \end{minipage}
%     \par
\end{minipage} \\[1cm]

\begin{minipage}{0.75\linewidth}
%     \renewcommand{\contentsname}{\large Document Contents \normalsize}
    \setcounter{tocdepth}{2}
    \centering
    \fontfamily{phv}\selectfont%
%     \renewcommand{\contentsname}{\large  \normalsize}
    \tableofcontents
%     \fontfamily{phv}\selectfont
%     \listoftables
\end{minipage}
% \fontsize{8}{10}\fontfamily{phv}\selectfont\noindent%
%
\thispagestyle{empty}
\newpage
\end{titlepage}

\mypagestyle
\fancypagestyle{plain}{\mypagestyle}

\renewcommand{\contentsname}{Table of Contents}
}

% Fake acronym for irregular plurals %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\makeatletter

\newcommand{\facp}{\@ifstar \facpStar \facpNoStar}

\newcommand{\facpNoStar}[1]{%
% Si el singular ja esta fet servir mostra la versio plural curta tambe
% Si no marca el singular com usat
% L'estat del plural no s'ha de canviar perque no surti despres a la llista
\expandafter\ifx\csname ac@#1\endcsname\AC@used %
\acs*{#1s}\acused{#1}%
\else
\acf*{#1s}\acused{#1}%
\fi
}

\newcommand{\facpStar}[1]{%
% Si el singular ja esta fet servir mostra la versio plural curta tambe
% Si no marca el singular com usat
% L'estat del plural no s'ha de canviar perque no surti despres a la llista
\expandafter\ifx\csname ac@#1\endcsname\AC@used %
\acs*{#1s}%
\else
\acf*{#1s}%
\fi
}

% Hi ha un warning perque es fa servir la cita del plural i
% no anira linkada al del singular pero no passa res (a menys que algu piqui :P)

\makeatother


% Visual mark for easy revision %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\makeatletter

\newcommand{\checkhere}[1]{%
\leavevmode\vbox to\z@{%
\vss%
\rlap{\vrule\raise .75em%
\hbox{\underbar{\normalfont\footnotesize\ttfamily \ding{43} #1}}}}%
\@latex@warning{A "checkhere" mark is still present and uncommented.}%
}
\makeatother

% Here goes %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\makeatletter
\newcommand{\hg}[1]{%
\noindent\ding{43} \colorbox{yellow}{\texttt{\footnotesize\uline{#1}}} \hfil \\ %
%\marginpar[left text]{right text} %
\@latex@warning{A suggestion is still present and uncommented.}%
}
\makeatother


% Cell dot-based alignments %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\newcolumntype{d}[1]{D{.}{.}{#1}}
\newcolumntype{.}{D{.}{.}{-1}}
%colored cells
\makeatletter
\newcolumntype{E}[1]{>{\boldmath\color{red}\DC@{.}{.}{#1}}c<{\DC@end}}
\newcolumntype{F}[1]{>{\boldmath\color{green}\DC@{.}{.}{#1}}c<{\DC@end}}
\makeatother


% To use in combination with latexdiff for things that are not diff'ed properly %%

\newcommand{\tableChanged}{{\protect\color{blue}\noindent\ding{43} This table has changed.}}

\newcommand{\figureChanged}{{\protect\color{blue}\noindent\ding{43} This figure has changed.}}

\newcommand{\equationChanged}{{\protect\color{blue}\noindent\ding{43} This equation has changed.}}

% Some useful things %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\newcommand{\argmax}{\operatornamewithlimits{argmax}}
\newcommand{\argmin}{\operatornamewithlimits{argmin}}
\newcommand{\median}{\operatornamewithlimits{median}}

\definecolor{good}{rgb}{0,.8,0}
\definecolor{bad}{rgb}{.9,0,0}

% Review tools %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Some macros provided by latexdiff are cloned here (so that latexdiff output can be copied into the document file directly).
\RequirePackage[normalem]{ulem}
\RequirePackage{color}\definecolor{RED}{rgb}{1,0,0}\definecolor{BLUE}{rgb}{0,0,1}

\newcommand{\DIFadd}[1]{{\protect\color{blue}\uwave{#1}}}
\newcommand{\DIFdel}[1]{{\protect\color{red}\sout{#1}}}

% Dissable latexdiff eye candy (uncomment following lines for easy pdf to word copy to spellcheck)
%\renewcommand{\DIFadd}[1]{{\protect{#1}}}
%\renewcommand{\DIFdel}[1]{{\protect{#1}}}

\providecommand{\DIFaddFL}[1]{\DIFadd{#1}}
\providecommand{\DIFdelFL}[1]{\DIFdel{#1}}

\providecommand{\DIFdelbegin}{}
\providecommand{\DIFdelend}{}
\providecommand{\DIFaddbegin}{}
\providecommand{\DIFaddend}{}
\providecommand{\DIFdelbeginFL}{}
\providecommand{\DIFdelendFL}{}
\providecommand{\DIFaddbeginFL}{}
\providecommand{\DIFaddendFL}{}

% To deal with reviewers... %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% To quote reviewers comments (\comment)
\definecolor{quote}{rgb}{.13,.33,.0}
\newcommand{\comment}[1]{%
\textcolor{quote}{\textit{#1}}%
}

% This is some code to preserve text color between page breaks while in framed mode
\makeatletter
\def\FrameCommand{%
\pdfcolorstack\@pdfcolorstack push{\current@color}%
\hspace{.3in} \vrule width 3pt%
\pdfcolorstack\@pdfcolorstack pop\relax%
\fboxrule=\FrameRule \fboxsep=\FrameSep \fbox%
}
\makeatother

% To display modifications in the document (use \DIFadd and \DIFdel here).
\newcommand{\modification}[1]{%
\MakeFramed {\advance\hsize -1.3in \FrameRestore}%
\noindent%
#1
\endMakeFramed
}

\newcommand{\remarknote}[1]{%
\vspace{1em}\noindent%
\framebox[\linewidth][l]{\hfill\textbf{#1}\hfill}%
\vspace{2em}%
}

\makeatletter
