This is a converted archive from the now defunct google code site ()

# Screenshots

[Link](http://imgur.com/a/NDNOK)

# Downloads

* VisualRBM Installer : [Link](http://goo.gl/gcLnAI visualrbm-setup.1.2.exe)
* MNIST Data
 * Training Images : https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/visual-rbm/mnist-train-images.idx
 * Training Labels : https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/visual-rbm/mnist-train-labels.idx
 * Testing Images : https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/visual-rbm/mnist-test-images.idx
 * Testing Labels : https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/visual-rbm/mnist-test-labels.idx
* MNIST Tutorial : [Link](http://goo.gl/giyPLH mnist-classifier.zip)

# News

News

1/10/2015

No new releases recently (and none coming for awhile) but don't worry, I am hard at work. I'm doing some major work on the GPU backend library, SiCKL ( https://code.google.com/p/sickl/'>https://code.google.com/p/sickl/ ):

* Changed SiCKL to use qmake so I can port to OSX and Linux
* Implementing an OpenCL frontend for SiCKL
* Making SiCKL a proper 64-bit shared library

This work is ongoing and is probably going to take awhile, but once I've finished I'll be able to easily port the command-line tools to other operating systems (targeting OSX and Linux). The eventual goal is to completely port VisualRBM to Qt on Windows, OSX and Linux and the third of you that come here on operating systems over than Windows will be able to leave happy :)


8/31/2014 - VisualRBM 1.2

* fixed issue in model JSON parsing and serialization code where parsing or serializing large models would cause crash
** cJSON replaced with new incremental JSON parser/serializer for models

8/9/2014 - VisualRBM 1.1

* fixed bug in installer where joinidx.exe was not being installed properly

8/05/2014 - VisualRBM 1.0

* created a snazzy NSIS installer for VisualRBM + Tools all in one package
** binary directory is automatically added to PATH variable
* added a Python IDX class for manipulating VisualRBM's IDX data files
* added support for Adadelta learning with tweakable decay parameter to RBM, AutoEncoder, and MLP trainers
* replaced momentum with Nesterov accelerated gradient to RBM, AutoEncoder and MLP trainers
* refactored dropout code in RBM and AutoEncoder to be slightly more efficient
** however, black speckles indicating which units have been dropped out are no longer visible
* added support for specifying random seed used to generate weights and other stochastic data
** prior to this version, the default seed was 1
* added optional -atlasSize command-line parameter to cltrain and VisualRBM which allows users to specify the allocation size for space on GPU used to store training data
** users experiencing crash with VisualRBM shortly after running should try running from command prompt "VisualRBM -atlasSize=VAL" with various values less than 512 until VisualRBM stops crashing on run. Once this number has been determined, update your VisualRBM shortcut to match this command string.
** default value is (and was) 512
* fixed bug in error calculation where Softmax was not being calculated correctly
* fixed crash bug in VisualRBM when user would load a second schedule after first schedule had completed

6/16/2014 - VisualRBM Build r333, Tools Build r333

* added a Tutorial section to the Downloads page which includes a zip containing a script and schedule files for training a classifier for the MNIST dataset
* added MLP training support to 'cltrain' command-line tool
* added new command-line tool 'buildmlp' which can construct an MLP model from a stack of RBMs, AutoEncoders, and MLPs
* added support to all training algorithms for 'Softmax' activation function
* added support to calchidden for handling models using 'Softmax' activation function
* fixed bug in SiCKL shader code generation which serialized float values without enough precision
** fixes bug in PRNG NextFloat() functions
** fixes crash bug on some AMD cards where 0 would be serialized as '0f' which isn't valid and prevented shader compilation
* fixed bug in AutoEncoder trainer where weight scaling due to Hidden dropout was calculated incorrectly (so AutoEncoder's should work much better now)
**fixed bug in DataAtlas where 'streaming' flag was not being reset on Initialize
** in VisualRBM if you loaded a large dataset that could not all fit in video RAM, did some training, and then loaded a much smaller dataset, the small dataset would get copied multiples times into video RAM and streamed in again and again during training
* fixed minor bug in VisualRBM where error logging would switch to scientific notation once error got sufficiently small which wasn't terrible readable

4/29/2014 - VisualRBM Build r309, Tools Build r309

* replaced 32 bit LCG PRNG with 128 bit XORShift128 PRNG
** All random number streams should be pretty much guaranteed to be uncorrelated
* fixed bug in AutoEncoder parsing logic where the Hidden and Output Activation functions would get swapped
* fixed bug where memory was not 16 byte aligned in calchidden, causing crash
* fixed bug where last feature would not get rendered when training an AutoEncoder
* fixed bug where Export button was enabled after Importing a model (pressing Export without Starting/Pausing first would cause crash))
* fixed crash caused by GPU memory fragmentation
** was triggered when changing the the minibatch size, or dataset used
** would cause DataAtlas to re-allocate GPU memory, now DataAtlas allocates a single large block (384 MB) and reuses it for lifetime of VisualRBM

3/15/2014 - VisualRBM Build r293, Tools Build r293

* Added support for actual AutoEncoder class, which uses tied encoder/decoder weights trained with BackPropagation
** AutoEncoder serialization identical to RestrictedBoltzmannMachine serialization, except type is 'AutoEncoder'
* Reduced max-size of DataAtlas to 384 mb (down from 512 mb)
** OOM crash was occurring for me on some datasets
* Added command-line tool 'image2csv' which converts an image to serialized CSV format (uses stb_image.c library)
* Added command-line tool 'shuffleidx' which shuffles an IDX file into a new random order

2/12/2014 - VisualRBM Build r268, Tools Build r268

* Integrated new OMLT backend (which is much easier to embed)
* Added initial support for AutoEncoders trained with backpropagation (encode/decode weights not currently tied)
* Changed model serialization to new JSON format
* Added abililty to create a JSON training schedule which can be used to automatically change training parameters during training
* Added support for Rectified Linear activation function to both RBM and AutoEncoder
* Amount of time delay before querying latest visible/hidden/weights dependent on how many we are visualizing
* Fixed bug where very large reconstruction error values would destroy the error graph

7/21/2013

* Implemented a simple 'infinite scroll' IDX viewer in JavaScript and HTML5 (tested in Chrome)
* Latest version can be found via the 'IDX Browser' link at the top or this URL: https://visual-rbm.googlecode.com/svn/trunk/source/Web/IDXView.xhtml
* Click and drag the data to navigate (will be adding arrow keys and page up/down soon)
* If you're interested in your own visualizations, IDX.js file reader can be found in that same source directory as well.

7/7/2013

No new releases recently, but a fair bit of work is being done behind the scenes. I've been working on implementing BackPropagation (with dropout!) with SiCKL and an associated MultilayerPerceptron class. If you've been following the source check-ins you will notice I've added a new VS Solution OMLT. That branch will be in a state of flux for awhile and will eventually completely replace the 'QuickBoltzmann' project in VisualRBM and clrbm.
* For the end users and developers here's what's coming down the line:
* OMLT is properly intended for embedding, so developers should have an easier time using the backend lib.
* OMLT includes a 'DataAtlas' class which takes care of streaming in rows of data from IDX data files, and converting them to the required SiCKL format used by the BackPropagation and RBMTrainer classes. The data is also now loaded into a single large texture, rather than thousands of little ones. As a result, GPU memory fragmentation is vastly reduced (MNIST training now uses about 200 MB of video memory now!)
* MultilayerPerceptron and eventually RestrictedBoltzmannMachine will be serialized in a JSON format (rather than the current ad-hoc binary format)

4/27/2013 - VisualRBM Build r168, Tools Build r168

* Fixed weight update shader to not perform weight update on weights to dropped-out units
* Fixed bug in error calculation, was not dividing by minibatch size
* Fixed bug in minibatch loading code for large datasets
* Initial weight standard deviation is now proportional to 1/sqrt(VisibleUnits) so that the hidden activations will not blow up when presented with larger visible input vectors

4/14/2013 - VisualRBM Build r162, Tools Build r162

* Improved performance by about 2-3x (depending on network architecture) by removing the periodic GPU->CPU weight transfer for use with the dynamic learning rate scaling (which is no longer performed)
** On average the weight scales calculated did not vary very much
** Learning rates will need to be tweaked a bit from values in previous VisualRBM versions
* Integrated the Simple Compute Kernel Library which encapsulates all GPU initialization and shader nonsense, so development of new features will be much easier in the future
* Speed improvement summary for MNIST over 100 epochs (clrbm in quiet mode on a GeForce GTX 660 Ti):
** 100 Hidden 500 Hidden 2000 Hidden 4000 Hidden
** Version 157 6m 38s 10m 35s 30m 7s 1h 13s
** Version 162 2m 1s 4m 21s 15m 26s 29m 52s
** Improvement 3.3x 2.4x 2x 2x

1/31/2013 - VisualRBM Build r157, Tools Build r157

* Fixed issue in IDX to GPU transfer method for large IDX files where last set of minibatches would contain the same minibatch repeatedly, resulting in overfitting

1/31/2013

There is a bug in the large-dataset related training code in VisualRBM and clrbm (see https://groups.google.com/d/msg/visual-rbm/tOo0e-RtUIA/BkeDCRripzYJ for details). A new published build should be up tonight that fixes this issue. Small datasets that are less than 1 GB (training + validation combined) are unaffected. Sorry for the inconvenience!

1/27/2013 - VisualRBM Build r154, Tools Build r154

* Added support to various command-line utilities for IDX files greater than 4 GB
* Added support to VisualRBM and clrbm for IDX files greater than 4 GB
* VisualRBM will no longer offers to automatically normalize data when using Gaussian visible units
* RBM file format no longer includes mean or standard deviation statistics about the data it was trained on (see FileFormats page for specs for the new format)

1/12/2013 - Tools Build r146

* Added csv2idx and idx2csv command line programs to make data conversion easier!

12/14/2012 - VisualRBM Build r138, Tools Build r138

* Shader source is now compiled directly into the executable, so NativeShaders folder is no longer required at runtime.

12/12/2012 - VisualRBM Build r128, Tools Build r131

* Added splitidx and calchidden command-line programs to the Tools.zip package.
* splitidx allows the user to breakup a single IDX data file into multiple subsets (for instance, dividing one large dataset into a training and test set).
* calchidden allows the user to calculate the hidden probabilities or the hidden activations of an entire dataset given a trained RBM model. The user can also specify the amount of visible dropout used so that the hidden unit activations don't get overloaded.
* Parameter Load/Save/Reset buttons are once again enabled when trainer is 'Paused'
* Fixed Epoch error calculation (was previously using the 250 iteration moving average used in the Graph tab).
* Fixed strange issue where printed error would be inconsistent between training runs during same session.
* Fixed Issue 4 where program would crash sometimes on Stop.

11/18/2012 - Tools Build r58

* Added joinidx,catidx and idxinfo command-line programs to the Tools.zip package.
* joinidx allows multiple IDX files with the same number of rows with different row length to be joined together. For example, you can 'join' a dataset with 10,000 length 100 rows and a dataset with 10,000 length 150 rows into a new dataset with 10,000 length 250 rows.
* catidx allows multiple IDX files with identical row dimensions to be concatenated together into a larger dataset. For example, you can 'cat' a dataset with 10,000 length 100 rows and a dataset with 20,000 length 100 rows into a new dataset with 30,000 100 rows.
* idxinfo prints out the header information of an IDX file.

11/14/2012

* Created a Google Groups forum for the project: https://groups.google.com/forum/#!forum/visual-rbm

11/11/2012 - VisualRBM Build r46, Tools Build r46

* When loading data for an RBM with Gaussian visible units, VisualRBM will now ask before normalizing the training data.
* Fixed two OpenGL bugs reported and fixed by step9899 (Issue 7 and Issue 8). VisualRBM should now run on AMD cards properly!.
* Fixed intermediates location in VisualRBM solution file.
* Split off clrbm to a seperate Tools solution so VisualRBM.sln will be less cluttered. (needs to be run in the same directory containing the 'NativeShaders' directory packaged with VisualRBM; this will be consolidated later).

10/14/2012 - VisualRBM Build r38

* Added ability to load a second dataset during while paused rather than having to export RBM, stop, import RBM and reload data
* Updated UI to include dropout parameters
* Parameters files can now be loaded when training is paused, but only training parameters will be updated, not model parameters. When training is stopped, all parameters will be updated.
* Changed default L2 regularization to 0.
* Fixed gaussian visible unit initialization bug; Was initializing visibles to the mean value when they should have been initialized with 0.0 (since the training data is normalized when using gaussian visible units). Gaussian visible unit models should now work better.
* Fixed some internal OpenGL errors to be compliant with 3.3
* Fixed reconstruction error calculation to take Visible Dropout into account
* Fixed bug in minibatch shuffling logic
* Fixed bug in vrbmparameter file parsing in VisualRBM

10/8/2012 - VisualRBM Build r34

* Added command line version, clrbm.exe (uses same parameters file as VisualRBM)
* Fixed bug where we would report insufficient OpenGL version available
* Added dropout support for visible units
* Added support for configuring dropout values in VisualRBM via the vrbmparameters file (ability to modify in the GUI will come with the next release)

9/10/2012 - VisualRBM Build r29

* Changed initial value of Hidden Unit count to 100 (had been accidentally changed to 0)
* Implemented dropout regularization (as described in Geoffrey Hinton's latest talk). Currently dropout rate is set to 0.5, so at any given time only half of the hidden units will be active. Cannot be changed in the UI yet and not saved off with parameters. During testing, visible activations should be halved when using all units so they get the same expected value.

9/9/2012 - VisualRBM Build r26

* Now checks to see GPU supports OpenGL 3.3 and will tell user so OpenGL context is now properly destroyed on program exit
* Tooltips are a bit better
* Bug Fixes

9/8/2012 - VisualRBM Build r20

* Reduced required OpenGL version to 3.3 which should cover more users
* Bug Fixes

9/7/2012

* Added C# helper classes for IDX and RBM filetypes

