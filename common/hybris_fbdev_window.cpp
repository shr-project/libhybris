/*
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "hybris_fbdev_window.h"

HybrisFbdevWindow::HybrisFbdevWindow() :
	m_fbdevWindow(new FbDevNativeWindow())
{
}

HybrisFbdevWindow::~HybrisFbdevWindow()
{
}

void HybrisFbdevWindow::Post(HybrisBufferHandle *handle)
{
}

EGLNativeWindowType HybrisFbdevWindow::getNativeWindow()
{
	return m_fbdevWindow;
}
