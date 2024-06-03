#include "UCjoystick.h"
#include <cstdio>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>


// AUCjoystick implementation
AUCjoystick::AUCjoystick()
{
    PrimaryActorTick.bCanEverTick = true;
    xAxis = 0;
    yAxis = 0;
    s = 0;
    fifoFd = -1;
    circularBuffer = new CircularBuffer(); // Initialize circular buffer
}
AUCjoystick::~AUCjoystick()
{ 
    if(fifoFd > 0){
        close(fifoFd);
    }
    delete circularBuffer;
}
void AUCjoystick::BeginPlay()
{
    Super::BeginPlay();
}

void AUCjoystick::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    readFifo();
    //parseJoystickData(xAxis, yAxis, s);

    // Do something with the data read from the buffer
}

void AUCjoystick::writeData(CircularBuffer* cb, const char* data, int length) {
    std::lock_guard<std::mutex> lock(cb->mtx);
    for (int i = 0; i < length; i++) {
        cb->buffer[cb->writeIndex] = data[i];
        cb->writeIndex = (cb->writeIndex + 1) % BUFFER_SIZE;
    }
}

void AUCjoystick::readData(CircularBuffer* cb, std::vector<std::string>& lines) {
    std::lock_guard<std::mutex> lock(cb->mtx);
    std::stringstream bufferStream;

    // Continue reading until the write index is caught up to the read index
    while (cb->readIndex != cb->writeIndex) {
        bufferStream << cb->buffer[cb->readIndex];
        cb->readIndex = (cb->readIndex + 1) % BUFFER_SIZE;
    }

    // Split bufferStream into individual lines and store in lines vector
    std::string line;
    while (std::getline(bufferStream, line)) {
        lines.push_back(line);
    }
}

void AUCjoystick::readFifo()
{
     

    const char* fifoPath = "/dev/ttyACM0";//"/dev/rfcomm0";
 

    // Check if the device exists
    if (access(fifoPath, F_OK) == -1) {
        UE_LOG(LogTemp, Warning, TEXT("Device %s does not exist: %s"), *FString(fifoPath), *FString(strerror(errno)));
        return;
    }
     

    fifoFd = open(fifoPath, O_RDONLY | O_NONBLOCK); //| O_NONBLOCK
    if (fifoFd < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to open FIFO %s"), *FString(strerror(errno)));
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    if ((bytesRead = read(fifoFd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0'; // Ensure null-termination
        writeData(circularBuffer, buffer, bytesRead); // Write data to circular buffer
    }else{
        UE_LOG(LogTemp, Error, TEXT("yazamadim %d ve byt %d ve buffer %s"), fifoFd, bytesRead, *FString(buffer));   
    }
    close(fifoFd);
}

void AUCjoystick::parseJoystickData(int& outX, int& outY, int& outSet)
{
    std::vector<std::string> lines;
    readData(circularBuffer, lines);

    // Process each line individually
    for (const auto& line : lines) {
        if (sscanf(line.c_str(), "%d %d %d", &outX, &outY, &outSet) == 3)
        {
            UE_LOG(LogTemp, Warning, TEXT("x %d - y %d - s %d"), outX, outY, outSet);

            FVector currentLocation = GetActorLocation();
            FVector newLocation = currentLocation;

            // Adjust speed multiplier
            float speedMultiplier = 100.0f;
            if (outX > 550 || outX < 450)
            {
                float normalizedX = (outX - 500) / 500.0f;
                newLocation.X += normalizedX * speedMultiplier * GetWorld()->GetDeltaSeconds();
            } 

            // Update the Y direction based on the comparison
            if (outY > 570 || outY < 470)
            {
                float normalizedY = (outY - 500) / 500.0f;
                newLocation.Y += normalizedY * speedMultiplier * GetWorld()->GetDeltaSeconds();
            }   

            // Update the actor's location
            SetActorLocation(newLocation);
        }
    }
}

void AUCjoystick::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}