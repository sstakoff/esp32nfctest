//
//  nfctestApp.swift
//  nfctest
//
//  Created by Stu Stakoff on 10/12/22.
//

import SwiftUI
import CoreNFC

@main
struct nfctestApp: App {
    
    private var mr = MyReader()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(mr)
        }
    }
}

class MyReader: NSObject, NFCNDEFReaderSessionDelegate, ObservableObject {
    
    @Published var msgs: [String] = []
    
    var session: NFCReaderSession?

    
    func readerSession(_ session: NFCNDEFReaderSession, didInvalidateWithError error: Error) {
        msgs.append("Got reader error")
        self.session = nil
    }
    
    func readerSession(_ session: NFCNDEFReaderSession, didDetectNDEFs messages: [NFCNDEFMessage]) {
        msgs.append("NDEF detected!")
    }
    
    func readerSessionDidBecomeActive(_ session: NFCNDEFReaderSession) {
        msgs.append("Reader is active")
    }
    
    func read() {
        if (!NFCReaderSession.readingAvailable) {
            msgs.append("Device does not support NFC")
            return
        }
        open()
        session?.begin()
    }
    
    private func open() {
        session = NFCNDEFReaderSession(delegate: self, queue: DispatchQueue.main, invalidateAfterFirstRead: false)
    }
    
    
    
   
    
}
