# Ogre Binding for IMGUI #

This is a raw Ogre binding for Imgui. No project/cmake file, no demo, just four source files, because If you're familiar with Ogre, integration should be pretty straightforward.


## License: ##

tl;dr : MIT license

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Dependencies ##

* Ogre
* IMGUI
* OIS

## Compatibility ##

This binding is compatible with both Ogre 1.x and 2.0.
Render systems supported are:

* D3D9
* D3D11
* GL3+

## Usage ##

### Compiling ###

Simply copy the files somewhere in your project or in your imgui project. include paths needed are:

* OgreMain/include
* OIS
* Boost if you are using it as Ogre thread provider

### Integration ###

Create and init the ImguiManager after your Ogre init:
```
ImguiManager::createSingleton();
ImguiManager::getSingleton().init(mSceneMgr,mOISKeyboardInput,mOISMouseInput);
```
Then in your render loop:
```
ImguiManager::getSingleton().newFrame(getDeltaTime(), Ogre::Rect(0,0,_getRenderWindow()->getWidth(),_getRenderWindow()->getHeight()));
```
And voilà !

You can then use imgui just like you want.

#### Note ####

You'll also need to transfer input events from your OIS Input listener to the Imgui manager.
For example:
```
bool MyInputManager::mouseMoved( const OIS::MouseEvent &arg )
{
    Ogre::ImguiManager::getSingleton().mouseMoved(arg);
}
```

## TODO ##

* Add proper comments