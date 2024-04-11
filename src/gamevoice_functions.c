/*
 * Copyright (c) 2012-2019 JoeBilly
 *
 * Game voice helper functions
 * gamevoice_functions.c
 * JoeBilly (joebilly@users.sourceforge.net)
 * https://github.com/ghoebilly/ts3gamevoice
 *
 *  This file is part of TeamSpeak 3 SideWinder Game Voice Plugin.
 *
 *  TeamSpeak 3 SideWinder Game Voice Plugin is free software:
 *	you can redistribute it and/or modify it under the terms of the
 *	GNU General Public License as published by the Free Software Foundation,
 *	either version 3 of the License, or (at your option) any later version.
 *
 *  TeamSpeak 3 SideWinder Game Voice Plugin is distributed in the hope
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with TeamSpeak 3 SideWinder Game Voice Plugin.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "usbHidCommunication.h"
#include "gamevoice_functions.h"

static struct UsbHidCommunication usbHidCommunicator;

static byte effectiveCommand = 0;
static byte lastCommandReceived = NONE;
static byte previousCommandReceived = NONE;
static byte lastFeatureSent = 0;
static BOOL featureSent = 0;

/* Gets the effective command applied to the device after a waitForCommand or waitForExternalCommand
 * Effective command contains buttons (Command) that are activated or deactivated (Action)
 */
static byte getEffectiveCommand(void)
{
	return effectiveCommand;
}

/* Gets the last command received from the device during a waitForCommand or waitForExternalCommand
 */
static byte getLastCommandReceived(void)
{
	return lastCommandReceived;
}

/* Gets the previous command received from the device after a waitForCommand or waitForExternalCommand
 */
static byte getPreviousCommandReceived(void)
{
	return previousCommandReceived;
}

/* Gets the last feature sent to the device during a forceFeature or sendFeature
 */
static byte getLastFeatureSent(void)
{
	return lastFeatureSent;
}

/* Determines whether the specified value is a new command and match the specified command.
 * A new command is a command different from the previous command received
 */
/*static BOOL isNewCommand(byte value, size_t command)
{
return !(previousCommandReceived & command) && (value & command);
}*/

/* Determines whether the last command is a new command and match the specified command.
 * A new command is a command different from the previous command received.
 */
/*static BOOL isNewLastCommand(size_t command)
{
return !(previousCommandReceived & command) && (lastCommandReceived & command);
}*/

/* Determines whether the specified button has been activated during a waitForcommand or waitForExternalCommand.
 * A button is activated if its a new command and different from the last feature sent.
 */
static BOOL isButtonActivated(byte command)
{
	return !(lastFeatureSent & command) && (effectiveCommand & command) && (effectiveCommand & ACTIVATED);
}

/* Determines whether the specified button is active on the device.
  */
static BOOL isButtonActive(byte command)
{
	return (usbHidCommunicator.getInputReport() & command);
}

/* Determines whether the specified button has been deactivated during a waitForcommand or waitForExternalCommand.
 * A button is deactivated if its a new command and different from the last feature sent.
 */
static BOOL isButtonDeactivated(byte command)
{
	return !(lastFeatureSent & command) && (effectiveCommand & command) && (effectiveCommand & DEACTIVATED);
}

/* Determines whether the specified button is inactive on the device.
 */
static BOOL isButtonInactive(byte command)
{
	return !(usbHidCommunicator.getInputReport() & command);
}

/* Determines whether the specified value is a new user command and match the specified command.
 * A new command is a command different from the previous command received.
 */
//static BOOL isNewExternalCommand(byte value, size_t command)
//{
//	return (lastFeatureSent != command) && !(previousCommandReceived & command) && (value & command);
//}
//
//static BOOL isNewLastExternalCommand(size_t command)
//{
//	return (lastFeatureSent != command) && !(previousCommandReceived & command) && (lastCommandReceived & command);
//}

/* Loads the device : find it and attach to it
*/
static BOOL loadDevice()
{
	usbHidCommunicator = CreateUsbHidCommunicator();
	usbHidCommunicator.initUsbHidCommunication();
	usbHidCommunicator.findDevice(0x045E, 0x003B);

	return usbHidCommunicator.isDeviceAttached();
}

/* Resets the device to its base state.
*/
static void resetDevice()
{
	effectiveCommand = 0;
	lastCommandReceived = NONE;
	previousCommandReceived = NONE;
	lastFeatureSent = 0;
	featureSent = 0;
	usbHidCommunicator.forceFeature(NONE);
	usbHidCommunicator.forceFeature(NONE);
}

/* Unload the device : reset and detach it
*/
static void unloadDevice()
{
	resetDevice();
	usbHidCommunicator.finalizeUsbHidCommunication();
}

/* Reads the last command received from the device
*/
static byte readCommand()
{
	return usbHidCommunicator.readFromTheInputBuffer(1);
}

/* Waits for a command from the device.
*/
static BOOL waitForCommand()
{
	char debugOutput[65];
	previousCommandReceived = lastCommandReceived;
	if (usbHidCommunicator.receiveCommand())
	{
		byte command;
		lastCommandReceived = readCommand();
		command = (previousCommandReceived ^ lastCommandReceived);

		if (command & COMMAND)
			command = COMMAND;

		if (command & lastCommandReceived)
			effectiveCommand = lastCommandReceived | ACTIVATED;
		else
			effectiveCommand = command | DEACTIVATED;

		snprintf(debugOutput, 65, "waitForCommand:lastFeatureSent:%u", lastFeatureSent);
		OutputDebugString(debugOutput);
		snprintf(debugOutput, 65, "waitForCommand:lastCommandReceived:%u", lastCommandReceived);
		OutputDebugString(debugOutput);
		snprintf(debugOutput, 65, "waitForCommand:effectiveCommand:%u", effectiveCommand);
		OutputDebugString(debugOutput);		
	}
	else
		return FALSE;

	return TRUE;
}

/* Waits for an external command from the device.
* An external command is a command different from the last feature sent.
*/
static BOOL waitForExternalCommand()
{
	BOOL result;
	do
	{
		featureSent = FALSE;
		result = waitForCommand();
		//char debugOutput[65];
		//snprintf(debugOutput, 65, "waitForExternalCommand:featureSent:%d", featureSent);
		//OutputDebugString(debugOutput);
		Sleep(5);
	} while (result && featureSent);

	return result;
}

/* Forces a feature to the device (sent immediately)
*/
static BOOL forceFeature(byte command)
{
	featureSent = usbHidCommunicator.forceFeature(command);
	if (featureSent)
		lastFeatureSent = command;
	else
		return FALSE;

	return TRUE;
}

/* Sends a feature to the device when its available
*/
static BOOL sendFeature(byte command)
{
	featureSent = usbHidCommunicator.sendFeature(command);
	if (featureSent)
		lastFeatureSent = command;
	else
		return FALSE;

	return TRUE;
}

/* Activate the specified button
*/
static BOOL activateButton(byte command)
{
	byte currentFeature;
	currentFeature = usbHidCommunicator.getInputReport();

	// Button already active
	if (currentFeature & command)
		return FALSE;

	return forceFeature(currentFeature | command);
}

/* Deactivate the specified button
*/
static BOOL deactivateButton(byte command)
{
	byte currentFeature;
	currentFeature = usbHidCommunicator.getInputReport();

	// Button already inactive
	if (!(currentFeature & command))
		return FALSE;

	return forceFeature(currentFeature ^ command);
}

/* Runs a clockwise led chase effect by activating & deactivating all device buttons sequentially
*/
static void runDeviceLedChase()
{
	sendFeature(CHANNEL_1);
	Sleep(75);
	sendFeature(CHANNEL_2);
	Sleep(75);
	sendFeature(CHANNEL_3);
	Sleep(75);
	sendFeature(CHANNEL_4);
	Sleep(75);
	sendFeature(NONE);
	sendFeature(COMMAND);
	Sleep(75);
	sendFeature(NONE);
	sendFeature(TEAM);
	Sleep(75);
	sendFeature(ALL);
	Sleep(75);
	sendFeature(NONE);
}

/* Blinks the device leds/button by activating & deactivating device buttons.
 */
static void blinkDevice()
{
	byte previousState = usbHidCommunicator.getInputReport();

	forceFeature(NONE);
	Sleep(75);
	forceFeature(CHANNEL_1 | CHANNEL_2 | CHANNEL_3 | CHANNEL_4);
	Sleep(100);
	forceFeature(NONE);
	Sleep(75);
	forceFeature(CHANNEL_1 | CHANNEL_2 | CHANNEL_3 | CHANNEL_4);
	Sleep(100);
	forceFeature(NONE);
	Sleep(75);
	forceFeature(CHANNEL_1 | CHANNEL_2 | CHANNEL_3 | CHANNEL_4);
	Sleep(100);
	forceFeature(NONE);
	Sleep(75);
	forceFeature(CHANNEL_1 | CHANNEL_2 | CHANNEL_3 | CHANNEL_4);
	Sleep(100);
	forceFeature(NONE);
	Sleep(75);
	forceFeature(previousState);
}

// GameVoiceFunctions factory
GameVoiceFunctions InitGameVoiceFunctions()
{
	GameVoiceFunctions gamevoiceFunctions;
	gamevoiceFunctions.blinkDevice = blinkDevice;
	gamevoiceFunctions.forceFeature = forceFeature;
	gamevoiceFunctions.getEffectiveCommand = getEffectiveCommand;
	gamevoiceFunctions.getLastCommandReceived = getLastCommandReceived;
	gamevoiceFunctions.getLastFeatureSent = getLastFeatureSent;
	gamevoiceFunctions.getPreviousCommandReceived = getPreviousCommandReceived;
	//gamevoiceFunctions.getPreviousState = getPreviousState;
	gamevoiceFunctions.isButtonActivated = isButtonActivated;
	gamevoiceFunctions.isButtonActive = isButtonActive;
	gamevoiceFunctions.isButtonDeactivated = isButtonDeactivated;
	gamevoiceFunctions.isButtonInactive = isButtonInactive;
	//gamevoiceFunctions.isNewCommand = isNewCommand;
	//gamevoiceFunctions.isNewLastCommand = isNewLastCommand;
	//gamevoiceFunctions.isNewExternalCommand = isNewExternalCommand;
	//gamevoiceFunctions.isNewLastExternalCommand = isNewLastExternalCommand;
	gamevoiceFunctions.loadDevice = loadDevice;
	gamevoiceFunctions.readCommand = readCommand;
	gamevoiceFunctions.resetDevice = resetDevice;
	gamevoiceFunctions.runDeviceLedChase = runDeviceLedChase;
	gamevoiceFunctions.sendFeature = sendFeature;
	gamevoiceFunctions.unloadDevice = unloadDevice;
	gamevoiceFunctions.waitForCommand = waitForCommand;
	gamevoiceFunctions.waitForExternalCommand = waitForExternalCommand;

	gamevoiceFunctions.activateButton = activateButton;
	gamevoiceFunctions.deactivateButton = deactivateButton;

	return gamevoiceFunctions;
}